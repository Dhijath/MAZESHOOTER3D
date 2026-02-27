/*==============================================================================

   フィールド描画用シェーダ実装 [Shaderfield.cpp]
														 Author : 51106
														 Date   : 2025/11/19
--------------------------------------------------------------------------------

   ・VS/PS を .cso から読み込み、入力レイアウトと各種定数バッファを用意
   ・World / View / Projection 行列とカラーを更新して描画パイプラインへ設定
   ・Cube / Model などの3D描画で使用

==============================================================================*/

#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader_field.h"

namespace
{
    // VS シェーダ関連
    ID3D11VertexShader* g_pVertexShader = nullptr;
    ID3D11InputLayout* g_pInputLayout = nullptr;

    ID3D11Buffer* g_pCBWorld = nullptr;   // b0
    ID3D11Buffer* g_pCBView = nullptr;   // b1
    ID3D11Buffer* g_pCBProj = nullptr;   // b2

    // PS シェーダ関連 (b0～b2)
    ID3D11Buffer* g_pCBDiffuse = nullptr; // b0 diffuse
    ID3D11Buffer* g_pCBAmbient = nullptr; // b1 ambient
    ID3D11Buffer* g_pCBDirectional = nullptr; // b2 directional

    ID3D11PixelShader* g_pPixelShader = nullptr;
    ID3D11SamplerState* g_pSamplerState = nullptr;

    ID3D11Device* g_pDevice = nullptr;
    ID3D11DeviceContext* g_pContext = nullptr;
}

/*==============================================================================

    初期化

==============================================================================*/
bool Shader_field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    HRESULT hr;
    g_pDevice = pDevice;
    g_pContext = pContext;

    /*---------------------------------------
        1. Vertex Shader 読み込み
    ----------------------------------------*/
    std::ifstream ifs_vs("Resource/Shader/shader_vertex_field.cso", std::ios::binary);
    if (!ifs_vs) {
        MessageBox(nullptr, "shader_vertex_field.cso が見つかりません", "error", MB_OK);
        return false;
    }
    ifs_vs.seekg(0, std::ios::end);
    size_t vsSize = ifs_vs.tellg();
    ifs_vs.seekg(0, std::ios::beg);

    std::vector<unsigned char> vsData(vsSize);
    ifs_vs.read((char*)vsData.data(), vsSize);

    hr = g_pDevice->CreateVertexShader(vsData.data(), vsSize, nullptr, &g_pVertexShader);
    if (FAILED(hr)) return false;

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsData.data(), vsSize, &g_pInputLayout);
    if (FAILED(hr)) return false;


    /*---------------------------------------
        2. VS 用 定数バッファ
        (World / View / Projection)
    ----------------------------------------*/
    D3D11_BUFFER_DESC bdVS{};
    bdVS.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bdVS.Usage = D3D11_USAGE_DEFAULT;
    bdVS.ByteWidth = sizeof(XMFLOAT4X4);

    g_pDevice->CreateBuffer(&bdVS, nullptr, &g_pCBWorld);
    g_pDevice->CreateBuffer(&bdVS, nullptr, &g_pCBView);
    g_pDevice->CreateBuffer(&bdVS, nullptr, &g_pCBProj);


    /*---------------------------------------
        3. Pixel Shader 読み込み
    ----------------------------------------*/
    std::ifstream ifs_ps("Resource/Shader/shader_pixel_field.cso", std::ios::binary);
    if (!ifs_ps) {
        MessageBox(nullptr, "shader_pixel_field.cso が見つかりません", "error", MB_OK);
        return false;
    }
    ifs_ps.seekg(0, std::ios::end);
    size_t psSize = ifs_ps.tellg();
    ifs_ps.seekg(0, std::ios::beg);

    std::vector<unsigned char> psData(psSize);
    ifs_ps.read((char*)psData.data(), psSize);

    hr = g_pDevice->CreatePixelShader(psData.data(), psSize, nullptr, &g_pPixelShader);
    if (FAILED(hr)) return false;


    /*---------------------------------------
        4. PS 用 buffer (b0, b1, b2)
    ----------------------------------------*/
    D3D11_BUFFER_DESC bdPS{};
    bdPS.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bdPS.Usage = D3D11_USAGE_DEFAULT;

    // b0 diffuse
    bdPS.ByteWidth = sizeof(XMFLOAT4);
    g_pDevice->CreateBuffer(&bdPS, nullptr, &g_pCBDiffuse);

    // b1 ambient
    bdPS.ByteWidth = sizeof(XMFLOAT4);
    g_pDevice->CreateBuffer(&bdPS, nullptr, &g_pCBAmbient);

    // b2 directional (vector + color)
    bdPS.ByteWidth = sizeof(XMFLOAT4) * 2;
    g_pDevice->CreateBuffer(&bdPS, nullptr, &g_pCBDirectional);


    /*---------------------------------------
        5. Sampler
    ----------------------------------------*/
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    g_pDevice->CreateSamplerState(&samp, &g_pSamplerState);

    return true;
}


/*==============================================================================

    終了処理

==============================================================================*/
void Shader_field_Finalize()
{
    SAFE_RELEASE(g_pSamplerState);

    SAFE_RELEASE(g_pCBDirectional);
    SAFE_RELEASE(g_pCBAmbient);
    SAFE_RELEASE(g_pCBDiffuse);

    SAFE_RELEASE(g_pCBProj);
    SAFE_RELEASE(g_pCBView);
    SAFE_RELEASE(g_pCBWorld);

    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}


void Shader_field_SetMatrix(const DirectX::XMMATRIX& matrix)
{
}

/*==============================================================================

    VS 定数バッファ更新

==============================================================================*/
void Shader_field_SetWorldMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCBWorld, 0, nullptr, &t, 0, 0);
}

void Shader_field_SetViewMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCBView, 0, nullptr, &t, 0, 0);
}

void Shader_field_SetProjectMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCBProj, 0, nullptr, &t, 0, 0);
}



/*==============================================================================

    PS 定数バッファ更新（必要なら呼ぶ）

==============================================================================*/
void Shader_field_SetDiffuse(const XMFLOAT4& c)
{
    g_pContext->UpdateSubresource(g_pCBDiffuse, 0, nullptr, &c, 0, 0);
}

void Shader_field_SetAmbient(const XMFLOAT4& c)
{
    g_pContext->UpdateSubresource(g_pCBAmbient, 0, nullptr, &c, 0, 0);
}

void Shader_field_SetDirectional(const XMFLOAT4* value2xFloat4)
{
    g_pContext->UpdateSubresource(g_pCBDirectional, 0, nullptr, value2xFloat4, 0, 0);
}


void Shader_field_3D_SetColor(const XMFLOAT4& color)
{
    Shader_field_SetDiffuse(color);   // 正解

}

/*==============================================================================

    描画開始

==============================================================================*/
void Shader_field_Begin()
{
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pContext->IASetInputLayout(g_pInputLayout);

    // VS 定数バッファ
    g_pContext->VSSetConstantBuffers(0, 1, &g_pCBWorld);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pCBView);
    g_pContext->VSSetConstantBuffers(2, 1, &g_pCBProj);

    // PS 定数バッファ (b0〜b2)
    g_pContext->PSSetConstantBuffers(0, 1, &g_pCBDiffuse);
    g_pContext->PSSetConstantBuffers(1, 1, &g_pCBAmbient);
    g_pContext->PSSetConstantBuffers(2, 1, &g_pCBDirectional);

    // サンプラ
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
}

