/*==============================================================================

   壁専用シェーダ管理 [WallShader.cpp]
                                                         Author : 51106
                                                         Date   : 2026/02/16
--------------------------------------------------------------------------------

   ■このファイルがやること
   ・壁専用HLSLシェーダーの管理
   ・UVRepeatによるテクスチャループ対応
   ・ライティング情報の保持と転送
   ・既存Shader3dには一切影響を与えない独立システム

==============================================================================*/

#include "WallShader.h"
#include "direct3d.h"
#include "texture.h"

#include <fstream>
#include <d3dcompiler.h>

using namespace DirectX;

namespace
{
    ID3D11VertexShader* g_pVS = nullptr;
    ID3D11PixelShader* g_pPS = nullptr;
    ID3D11InputLayout* g_pLayout = nullptr;

    ID3D11Buffer* g_pCB_World = nullptr;
    ID3D11Buffer* g_pCB_View = nullptr;
    ID3D11Buffer* g_pCB_Proj = nullptr;
    ID3D11Buffer* g_pCB_Color = nullptr;
    ID3D11Buffer* g_pCB_UV = nullptr;
    ID3D11Buffer* g_pCB_Light = nullptr;

    ID3D11Device* g_pDevice = nullptr;
    ID3D11DeviceContext* g_pContext = nullptr;

    struct WallLightData
    {
        XMFLOAT3 ambient;
        float padding1;
        XMFLOAT4 directional_direction;
        XMFLOAT4 directional_color;
        XMFLOAT3 camera_position;
        float specular_power;
        XMFLOAT4 specular_color;
    };

    WallLightData g_WallLight{};
}

static bool LoadShader(const char* path, ID3DBlob** blob)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    ifs.seekg(0, std::ios::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    D3DCreateBlob(size, blob);
    ifs.read((char*)(*blob)->GetBufferPointer(), size);
    return true;
}

//==============================================================================
// 初期化
//
// ■役割
// ・壁専用シェーダーのロードとコンパイル
// ・頂点レイアウトの作成
// ・各種定数バッファの作成
//
// ■引数
// ・dev : D3Dデバイス
// ・ctx : デバイスコンテキスト
//==============================================================================
bool WallShader_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    g_pDevice = dev;
    g_pContext = ctx;

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    LoadShader("resource/shader/shader_vertex_wall.cso", &vsBlob);
    LoadShader("resource/shader/shader_pixel_wall_lit.cso", &psBlob);

    g_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVS);
    g_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPS);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
    };

    g_pDevice->CreateInputLayout(
        layout, 4,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &g_pLayout
    );

    SAFE_RELEASE(vsBlob);
    SAFE_RELEASE(psBlob);

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(XMFLOAT4X4);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_World);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_View);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_Proj);

    bd.ByteWidth = sizeof(XMFLOAT4);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_Color);

    bd.ByteWidth = sizeof(XMFLOAT2) * 2;
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_UV);

    bd.ByteWidth = sizeof(WallLightData);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pCB_Light);

    return true;
}

//==============================================================================
// 終了処理
//
// ■役割
// ・各種リソースの解放
//==============================================================================
void WallShader_Finalize()
{
    SAFE_RELEASE(g_pCB_Light);
    SAFE_RELEASE(g_pCB_UV);
    SAFE_RELEASE(g_pCB_Color);
    SAFE_RELEASE(g_pCB_Proj);
    SAFE_RELEASE(g_pCB_View);
    SAFE_RELEASE(g_pCB_World);

    SAFE_RELEASE(g_pLayout);
    SAFE_RELEASE(g_pPS);
    SAFE_RELEASE(g_pVS);

    g_pDevice = nullptr;
    g_pContext = nullptr;
}

//==============================================================================
// シェーダー開始
//
// ■役割
// ・壁描画用のシェーダーとバッファをパイプラインにセット
// ・ライト情報を定数バッファに転送
//==============================================================================
void WallShader_Begin()
{
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->PSSetShader(g_pPS, nullptr, 0);
    g_pContext->IASetInputLayout(g_pLayout);

    g_pContext->VSSetConstantBuffers(0, 1, &g_pCB_World);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pCB_View);
    g_pContext->VSSetConstantBuffers(2, 1, &g_pCB_Proj);

    g_pContext->PSSetConstantBuffers(0, 1, &g_pCB_Color);
    g_pContext->PSSetConstantBuffers(1, 1, &g_pCB_UV);

    g_pContext->UpdateSubresource(g_pCB_Light, 0, nullptr, &g_WallLight, 0, 0);
    g_pContext->PSSetConstantBuffers(2, 1, &g_pCB_Light);
}

//==============================================================================
// ワールド行列設定
//
// ■引数
// ・m : ワールド変換行列
//==============================================================================
void WallShader_SetWorldMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCB_World, 0, nullptr, &t, 0, 0);
}

//==============================================================================
// 色設定
//
// ■引数
// ・c : ディフューズカラー（RGBA）
//==============================================================================
void WallShader_SetColor(const XMFLOAT4& c)
{
    g_pContext->UpdateSubresource(g_pCB_Color, 0, nullptr, &c, 0, 0);
}

//==============================================================================
// UVリピート設定
//
// ■役割
// ・テクスチャのタイリング繰り返し回数を設定
//
// ■引数
// ・uv : UV繰り返し回数（X, Y）
//==============================================================================
void WallShader_SetUVRepeat(const XMFLOAT2& uv)
{
    g_pContext->UpdateSubresource(g_pCB_UV, 0, nullptr, &uv, 0, 0);
}

//==============================================================================
// テクスチャ設定
//
// ■引数
// ・texID : テクスチャID
//==============================================================================
void WallShader_SetTexture(int texID)
{
    Set_Texture(texID, 0);
}

//==============================================================================
// ビュー行列設定
//
// ■引数
// ・m : ビュー変換行列
//==============================================================================
void WallShader_SetViewMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCB_View, 0, nullptr, &t, 0, 0);
}

//==============================================================================
// プロジェクション行列設定
//
// ■引数
// ・m : 投影変換行列
//==============================================================================
void WallShader_SetProjectMatrix(const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    g_pContext->UpdateSubresource(g_pCB_Proj, 0, nullptr, &t, 0, 0);
}

//==============================================================================
// 環境光設定
//
// ■役割
// ・壁シェーダー用の環境光色を保持
// ・実際の転送はWallShader_Begin()で行われる
//
// ■引数
// ・color : 環境光の色（RGB）
//==============================================================================
void WallShader_SetAmbient(const XMFLOAT3& color)
{
    g_WallLight.ambient = color;
}

//==============================================================================
// 平行光源設定
//
// ■役割
// ・壁シェーダー用の平行光源情報を保持
// ・実際の転送はWallShader_Begin()で行われる
//
// ■引数
// ・direction : 光の向き
// ・color     : 光の色（RGBA）
//==============================================================================
void WallShader_SetDirectional(const XMFLOAT4& direction, const XMFLOAT4& color)
{
    g_WallLight.directional_direction = direction;
    g_WallLight.directional_color = color;
}

//==============================================================================
// 鏡面反射光設定
//
// ■役割
// ・壁シェーダー用の鏡面反射情報を保持
// ・実際の転送はWallShader_Begin()で行われる
//
// ■引数
// ・camera_position : カメラの位置
// ・specular_power  : ハイライトの鋭さ
// ・specular_color  : 鏡面反射の色（RGBA）
//==============================================================================
void WallShader_SetSpecular(const XMFLOAT3& camera_position, float specular_power, const XMFLOAT4& specular_color)
{
    g_WallLight.camera_position = camera_position;
    g_WallLight.specular_power = specular_power;
    g_WallLight.specular_color = specular_color;
}