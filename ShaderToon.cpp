/*==============================================================================

   トゥーンシェーダー管理 [ShaderToon.cpp]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   ・Shader3d.cpp をベースに、トゥーン PS とアウトライン VS/PS を追加管理
   ・VS（通常）は shader_vertex_3d.cso をそのまま流用
   ・アウトラインは shader_vertex_outline.cso + shader_pixel_outline.cso
   ・トゥーン本体は shader_vertex_3d.cso + shader_pixel_toon.cso

   【2パス描画の流れ】
     1パス目: ShaderToon_BeginOutline()
              → CullFront + アウトライン VS/PS
              → ModelDraw()
     2パス目: ShaderToon_Begin()
              → CullBack + 通常 VS + トゥーン PS
              → ModelDraw()

==============================================================================*/
#include "ShaderToon.h"
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

namespace
{
    //--------------------------------------------------------------------------
    // デバイス
    //--------------------------------------------------------------------------
    ID3D11Device* g_pDevice = nullptr;
    ID3D11DeviceContext* g_pContext = nullptr;

    //--------------------------------------------------------------------------
    // 通常 VS（shader_vertex_3d.cso 流用）
    //--------------------------------------------------------------------------
    ID3D11VertexShader* g_pVS = nullptr;
    ID3D11InputLayout* g_pLayout = nullptr;

    //--------------------------------------------------------------------------
    // トゥーン PS（shader_pixel_toon.cso）
    //--------------------------------------------------------------------------
    ID3D11PixelShader* g_pToonPS = nullptr;

    //--------------------------------------------------------------------------
    // アウトライン VS/PS
    //--------------------------------------------------------------------------
    ID3D11VertexShader* g_pOutlineVS = nullptr;
    ID3D11PixelShader* g_pOutlinePS = nullptr;

    //--------------------------------------------------------------------------
    // ラスタライザーステート
    //--------------------------------------------------------------------------
    ID3D11RasterizerState* g_pRSCullBack = nullptr; // 通常描画用
    ID3D11RasterizerState* g_pRSCullFront = nullptr; // アウトライン用（裏面描画）

    //--------------------------------------------------------------------------
    // 定数バッファ（VS 共通 : World/View/Proj）
    //--------------------------------------------------------------------------
    ID3D11Buffer* g_pCB_World = nullptr;
    ID3D11Buffer* g_pCB_View = nullptr;
    ID3D11Buffer* g_pCB_Proj = nullptr;

    //--------------------------------------------------------------------------
    // 定数バッファ（PS : diffuse color）
    //--------------------------------------------------------------------------
    ID3D11Buffer* g_pCB_Color = nullptr;

    //--------------------------------------------------------------------------
    // 定数バッファ（アウトライン VS b3 : 太さ）
    //--------------------------------------------------------------------------
    struct OutlineVSParam
    {
        float outlineWidth;
        float dummy[3];
    };
    ID3D11Buffer* g_pCB_OutlineWidth = nullptr;

    //--------------------------------------------------------------------------
    // 定数バッファ（アウトライン PS b0 : 色）
    //--------------------------------------------------------------------------
    ID3D11Buffer* g_pCB_OutlineColor = nullptr;

    //--------------------------------------------------------------------------
    // 定数バッファ（トゥーン PS b7 : 階調パラメータ）
    //--------------------------------------------------------------------------
    struct ToonParam
    {
        int   toonSteps;
        float toonSpecularSize;
        float toonRimPower;
        float toonRimThreshold;
    };
    ID3D11Buffer* g_pCB_ToonParam = nullptr;

    //--------------------------------------------------------------------------
    // ヘルパー：cso ファイルを読んでシェーダーバイナリを返す
    //--------------------------------------------------------------------------
    static bool LoadCSO(const char* path, unsigned char** ppData, size_t* pSize)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return false;

        ifs.seekg(0, std::ios::end);
        *pSize = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        *ppData = new unsigned char[*pSize];
        ifs.read(reinterpret_cast<char*>(*ppData), *pSize);
        return true;
    }

    //--------------------------------------------------------------------------
    // ヘルパー：float4x4 サイズの定数バッファ作成
    //--------------------------------------------------------------------------
    static ID3D11Buffer* CreateCB(UINT byteWidth)
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = byteWidth;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage = D3D11_USAGE_DEFAULT;

        ID3D11Buffer* pBuf = nullptr;
        g_pDevice->CreateBuffer(&desc, nullptr, &pBuf);
        return pBuf;
    }
}

//==============================================================================
// 初期化
//==============================================================================
bool ShaderToon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (!pDevice || !pContext) return false;
    g_pDevice = pDevice;
    g_pContext = pContext;

    HRESULT hr;
    unsigned char* pData = nullptr;
    size_t         size = 0;

    //--------------------------------------------------------------------------
    // 通常 VS（shader_vertex_3d.cso）を流用
    //--------------------------------------------------------------------------
    if (!LoadCSO("resource/shader/shader_vertex_3d.cso", &pData, &size))
    {
        MessageBox(nullptr, "shader_vertex_3d.cso が見つかりません (ShaderToon)", "error", MB_OK);
        return false;
    }

    hr = g_pDevice->CreateVertexShader(pData, size, nullptr, &g_pVS);
    if (FAILED(hr)) { delete[] pData; return false; }

    // 入力レイアウト（Shader3d と同一）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pData, size, &g_pLayout);
    delete[] pData;
    if (FAILED(hr)) return false;

    //--------------------------------------------------------------------------
    // トゥーン PS（shader_pixel_toon.cso）
    //--------------------------------------------------------------------------
    if (!LoadCSO("resource/shader/shader_pixel_toon.cso", &pData, &size))
    {
        MessageBox(nullptr, "shader_pixel_toon.cso が見つかりません", "error", MB_OK);
        return false;
    }
    hr = g_pDevice->CreatePixelShader(pData, size, nullptr, &g_pToonPS);
    delete[] pData;
    if (FAILED(hr)) return false;

    //--------------------------------------------------------------------------
    // アウトライン VS（shader_vertex_outline.cso）
    //--------------------------------------------------------------------------
    if (!LoadCSO("resource/shader/shader_vertex_outline.cso", &pData, &size))
    {
        MessageBox(nullptr, "shader_vertex_outline.cso が見つかりません", "error", MB_OK);
        return false;
    }
    hr = g_pDevice->CreateVertexShader(pData, size, nullptr, &g_pOutlineVS);
    delete[] pData;
    if (FAILED(hr)) return false;

    //--------------------------------------------------------------------------
    // アウトライン PS（shader_pixel_outline.cso）
    //--------------------------------------------------------------------------
    if (!LoadCSO("resource/shader/shader_pixel_outline.cso", &pData, &size))
    {
        MessageBox(nullptr, "shader_pixel_outline.cso が見つかりません", "error", MB_OK);
        return false;
    }
    hr = g_pDevice->CreatePixelShader(pData, size, nullptr, &g_pOutlinePS);
    delete[] pData;
    if (FAILED(hr)) return false;

    //--------------------------------------------------------------------------
    // ラスタライザーステート
    //--------------------------------------------------------------------------
    {
        D3D11_RASTERIZER_DESC rsDesc{};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_BACK;
        rsDesc.DepthClipEnable = TRUE;
        g_pDevice->CreateRasterizerState(&rsDesc, &g_pRSCullBack);

        rsDesc.CullMode = D3D11_CULL_FRONT; // アウトライン用
        g_pDevice->CreateRasterizerState(&rsDesc, &g_pRSCullFront);
    }

    //--------------------------------------------------------------------------
    // 定数バッファ作成
    //--------------------------------------------------------------------------
    g_pCB_World = CreateCB(sizeof(XMFLOAT4X4));  // VS b0
    g_pCB_View = CreateCB(sizeof(XMFLOAT4X4));  // VS b1
    g_pCB_Proj = CreateCB(sizeof(XMFLOAT4X4));  // VS b2
    g_pCB_Color = CreateCB(sizeof(XMFLOAT4));    // PS b0 (diffuse)
    g_pCB_OutlineWidth = CreateCB(sizeof(OutlineVSParam)); // outline VS b3
    g_pCB_OutlineColor = CreateCB(sizeof(XMFLOAT4));    // outline PS b0
    g_pCB_ToonParam = CreateCB(sizeof(ToonParam));   // toon PS b7

    //--------------------------------------------------------------------------
    // デフォルト値をセット
    //--------------------------------------------------------------------------
    ShaderToon_SetToonParam(3, 0.8f, 5.0f, 0.4f);
    ShaderToon_SetOutlineWidth(0.002f);
    ShaderToon_SetOutlineColor({ 0.0f, 0.0f, 0.0f, 1.0f });

    return true;
}

//==============================================================================
// 終了
//==============================================================================
void ShaderToon_Finalize()
{
    SAFE_RELEASE(g_pRSCullBack);
    SAFE_RELEASE(g_pRSCullFront);
    SAFE_RELEASE(g_pCB_ToonParam);
    SAFE_RELEASE(g_pCB_OutlineColor);
    SAFE_RELEASE(g_pCB_OutlineWidth);
    SAFE_RELEASE(g_pCB_Color);
    SAFE_RELEASE(g_pCB_Proj);
    SAFE_RELEASE(g_pCB_View);
    SAFE_RELEASE(g_pCB_World);
    SAFE_RELEASE(g_pOutlinePS);
    SAFE_RELEASE(g_pOutlineVS);
    SAFE_RELEASE(g_pToonPS);
    SAFE_RELEASE(g_pLayout);
    SAFE_RELEASE(g_pVS);
}

//==============================================================================
// パラメータ設定
//==============================================================================
void ShaderToon_SetToonParam(int steps, float specularSize, float rimPower, float rimThreshold)
{
    ToonParam param;
    param.toonSteps = steps;
    param.toonSpecularSize = specularSize;
    param.toonRimPower = rimPower;
    param.toonRimThreshold = rimThreshold;
    g_pContext->UpdateSubresource(g_pCB_ToonParam, 0, nullptr, &param, 0, 0);
}

void ShaderToon_SetOutlineWidth(float width)
{
    OutlineVSParam param;
    param.outlineWidth = width;
    param.dummy[0] = param.dummy[1] = param.dummy[2] = 0.0f;
    g_pContext->UpdateSubresource(g_pCB_OutlineWidth, 0, nullptr, &param, 0, 0);
}

void ShaderToon_SetOutlineColor(const XMFLOAT4& color)
{
    g_pContext->UpdateSubresource(g_pCB_OutlineColor, 0, nullptr, &color, 0, 0);
}

//==============================================================================
// 行列 / カラー設定
//==============================================================================
void ShaderToon_SetWorldMatrix(const XMMATRIX& matrix)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pCB_World, 0, nullptr, &t, 0, 0);
}

void ShaderToon_SetViewMatrix(const XMMATRIX& matrix)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pCB_View, 0, nullptr, &t, 0, 0);
}

void ShaderToon_SetProjectMatrix(const XMMATRIX& matrix)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pCB_Proj, 0, nullptr, &t, 0, 0);
}

void ShaderToon_SetColor(const XMFLOAT4& color)
{
    g_pContext->UpdateSubresource(g_pCB_Color, 0, nullptr, &color, 0, 0);
}

//==============================================================================
// トゥーン本体パス（2パス目）
//==============================================================================
void ShaderToon_Begin()
{
    // 通常カリング（裏面除去）
    g_pContext->RSSetState(g_pRSCullBack);

    // VS : shader_vertex_3d.cso 流用
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->IASetInputLayout(g_pLayout);

    // VS 定数バッファ（World/View/Proj）
    g_pContext->VSSetConstantBuffers(0, 1, &g_pCB_World);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pCB_View);
    g_pContext->VSSetConstantBuffers(2, 1, &g_pCB_Proj);

    // PS : shader_pixel_toon.cso
    g_pContext->PSSetShader(g_pToonPS, nullptr, 0);

    // PS b0 : diffuse color
    g_pContext->PSSetConstantBuffers(0, 1, &g_pCB_Color);

    // PS b7 : トゥーンパラメータ
    g_pContext->PSSetConstantBuffers(7, 1, &g_pCB_ToonParam);
}

//==============================================================================
// アウトラインパス（1パス目）
//==============================================================================
void ShaderToon_BeginOutline()
{
    // 前面カリング（裏面だけ描く）
    g_pContext->RSSetState(g_pRSCullFront);

    // VS : shader_vertex_outline.cso
    g_pContext->VSSetShader(g_pOutlineVS, nullptr, 0);
    g_pContext->IASetInputLayout(g_pLayout); // 入力レイアウトは共通

    // VS 定数バッファ（World/View/Proj）
    g_pContext->VSSetConstantBuffers(0, 1, &g_pCB_World);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pCB_View);
    g_pContext->VSSetConstantBuffers(2, 1, &g_pCB_Proj);

    // VS b3 : アウトライン太さ
    g_pContext->VSSetConstantBuffers(3, 1, &g_pCB_OutlineWidth);

    // PS : shader_pixel_outline.cso（黒べた塗り）
    g_pContext->PSSetShader(g_pOutlinePS, nullptr, 0);

    // PS b0 : アウトライン色
    g_pContext->PSSetConstantBuffers(0, 1, &g_pCB_OutlineColor);
}