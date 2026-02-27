/*==============================================================================

   ビルボード描画 [billboard.cpp]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
==============================================================================*/

#include "billboard.h"
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader_billboard.h"
#include "key_logger.h"
#include "mouse.h"
#include "texture.h"
#include "Player_Camera.h"

using namespace DirectX;

// 匿名名前空間：この cpp 内だけで使う変数
namespace
{
    constexpr int NUM_VERTEX = 4;                // ビルボード用頂点数（四角ポリゴン）
    ID3D11Buffer* g_pVertexBuffer = nullptr;     // 頂点バッファ
}

// ビルボード用頂点構造体
struct Vertex3D
{
    XMFLOAT3 position; // 頂点座標（ローカル）
    XMFLOAT4 color;    // 頂点カラー
    XMFLOAT2 uv;       // UV 座標
};

//==============================================================================
// 初期化：シェーダと頂点バッファの準備
//==============================================================================
void Billboard_Initialize()
{
    // ビルボード専用シェーダ初期化
    Shader_Billboard_Initialize();



    // ひとつの正方形ポリゴン（中心原点・サイズ 1x1）の頂点
    Vertex3D Vertex[] =
    {
        { { -0.5f,  0.5f, 0.0f }, { 1.0f,1.0f,1.0f,1.0f }, { 0.0f, 0.0f } }, // 左上
        { {  0.5f,  0.5f, 0.0f }, { 1.0f,1.0f,1.0f,1.0f }, { 1.0f, 0.0f } }, // 右上
        { { -0.5f, -0.5f, 0.0f }, { 1.0f,1.0f,1.0f,1.0f }, { 0.0f, 1.0f } }, // 左下
        { {  0.5f, -0.5f, 0.0f }, { 1.0f,1.0f,1.0f,1.0f }, { 1.0f, 1.0f } }, // 右下
    };

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;              // GPU 専用（CPU からは書き換えない）
    bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = Vertex;

    Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

//==============================================================================
// 終了処理：頂点バッファ・シェーダ解放
//==============================================================================
void Billboard_Finalize(void)
{
    SAFE_RELEASE(g_pVertexBuffer);
    Shader_Billboard_Finalize();
}

//==============================================================================
// ビルボード描画（テクスチャ全体を使用）
// texID   : 使用するテクスチャ ID
// position: ワールド座標（足元基準で置ける）
// scale   : X,Y のスケール（大きさ）
// pivot   : ピボット（ローカル原点オフセット） 0,0=左上 / 0.5,0.5=中央
//==============================================================================
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT2& pivot)
{
    // UV はテクスチャ全体を使用（1.0 x 1.0）
    Shader_Billboard_SetUVParameter({ { 1.0f, 1.0f }, { 0.0f, 0.0f } });

    // シェーダーを描画パイプラインに設定
    Shader_Billboard_Begin();

    // カラー（白）→ テクスチャ色そのまま
    Shader_Billboard_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // 使用するテクスチャをセット
    Set_Texture(texID);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3D);
    UINT offset = 0;
    Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    Direct3D_GetContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

    // pivot 分をローカル座標系でオフセットする行列
    XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

    // カメラのビュー行列を取得（向きだけを使う）
    XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
    CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;

    // 回転だけの逆行列（転置でOK）
    XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));

    // スケール
    XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);

    // 　ワールド平行移動は「positionそのまま」
    //   pivot をワールドに足すと基準がズレるので禁止
    XMMATRIX mtxt = XMMatrixTranslation(position.x, position.y, position.z);

    // ワールド行列をシェーダへ設定
    Shader_Billboard_SetWorldMatrix(pivot_offset * mtxs * iv * mtxt);

    // TriangleStrip で四角形を描画
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画
    Direct3D_GetContext()->Draw(NUM_VERTEX, 0);
}

//==============================================================================
// ビルボード描画（テクスチャの一部分を切り出し）
// tex_cut : { x, y, w, h } テクスチャ上の切り出し矩形（ピクセル単位）
//==============================================================================
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT4& tex_cut,
    const DirectX::XMFLOAT2& pivot)
{
    // シェーダーを描画パイプラインに設定
    Shader_Billboard_Begin();

    // テクスチャサイズから UV を計算
    float tex_w = static_cast<float>(Texture_Width(texID));
    float tex_h = static_cast<float>(Texture_Height(texID));

    float uv_x = tex_cut.x / tex_w;
    float uv_y = tex_cut.y / tex_h;
    float uv_w = tex_cut.z / tex_w;
    float uv_h = tex_cut.w / tex_h;

    // 使用範囲の UV を設定（サイズ + オフセット）
    Shader_Billboard_SetUVParameter({ { uv_w, uv_h }, { uv_x, uv_y } });

    // カラー（白）→ テクスチャそのまま
    Shader_Billboard_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // テクスチャ設定
    Set_Texture(texID);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3D);
    UINT offset = 0;
    Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    Direct3D_GetContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

    // pivot オフセット（ローカル）
    XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

    // カメラの向きだけを取り出したビュー行列
    XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
    CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;

    // 回転部分の逆行列として転置を使用
    XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));

    // スケール
    XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);

    // 　ワールド平行移動は position そのまま
    XMMATRIX mtxt = XMMatrixTranslation(position.x, position.y, position.z);

    // 最終的なワールド行列
    Shader_Billboard_SetWorldMatrix(pivot_offset * mtxs * iv * mtxt);

    // TriangleStrip で描画
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // ポリゴン描画命令発行
    Direct3D_GetContext()->Draw(NUM_VERTEX, 0);
}


//==============================================================================
// ビルボード描画（色付き・テクスチャ切り出し版）
//==============================================================================
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT4& color,      // 　 色パラメータ
    const DirectX::XMFLOAT4& tex_cut,
    const DirectX::XMFLOAT2& pivot)
{
    // シェーダーを描画パイプラインに設定
    Shader_Billboard_Begin();

    // テクスチャサイズから UV を計算
    float tex_w = static_cast<float>(Texture_Width(texID));
    float tex_h = static_cast<float>(Texture_Height(texID));

    float uv_x = tex_cut.x / tex_w;
    float uv_y = tex_cut.y / tex_h;
    float uv_w = tex_cut.z / tex_w;
    float uv_h = tex_cut.w / tex_h;

    // 使用範囲の UV を設定
    Shader_Billboard_SetUVParameter({ { uv_w, uv_h }, { uv_x, uv_y } });

    //  色を設定（テクスチャと乗算される）
    Shader_Billboard_SetColor(color);

    // テクスチャ設定
    Set_Texture(texID);

    // 頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex3D);
    UINT offset = 0;
    Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    Direct3D_GetContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

    // pivot オフセット
    XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

    // カメラの向きだけを取り出したビュー行列
    XMFLOAT4X4 CameraMatrix = Player_Camera_GetViewMatrix();
    CameraMatrix._41 = CameraMatrix._42 = CameraMatrix._43 = 0.0f;

    // 回転部分の逆行列として転置を使用
    XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&CameraMatrix));

    // スケール
    XMMATRIX mtxs = XMMatrixScaling(scale.x, scale.y, 1.0f);

    // ワールド平行移動
    XMMATRIX mtxt = XMMatrixTranslation(position.x, position.y, position.z);

    // 最終的なワールド行列
    Shader_Billboard_SetWorldMatrix(pivot_offset * mtxs * iv * mtxt);

    // TriangleStrip で描画
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // ポリゴン描画命令発行
    Direct3D_GetContext()->Draw(NUM_VERTEX, 0);
}