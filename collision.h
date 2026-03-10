/*==============================================================================

   当たり判定 [collision.h]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
==============================================================================*/

#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>
#include "debug_text.h"

// D3D の構造体前方宣言（ヘッダで <d3d11.h> を引きたくない場合用）
struct ID3D11Device;
struct ID3D11DeviceContext;

//-----------------------------
// 2D 円
//-----------------------------
struct Circle
{
    DirectX::XMFLOAT2 center; // 中心座標
    float             radius; // 半径
};

//-----------------------------
// 2D 矩形（AABB）
//-----------------------------
struct Box
{
    DirectX::XMFLOAT2 center;     // 中心座標
    float             halfWidth;  // 水平方向の半幅
    float             halfHeight; // 垂直方向の半幅
};

//-----------------------------
// 3D AABB（Axis Aligned Bounding Box）
//-----------------------------
struct AABB
{
    DirectX::XMFLOAT3 min; // 最小座標（左下奥）
    DirectX::XMFLOAT3 max; // 最大座標（右上手前）

    // 中心座標を計算して返す
    DirectX::XMFLOAT3 GetCenter() const
    {
        DirectX::XMFLOAT3 center;
        center.x = min.x + (max.x - min.x) * 0.5f;
        center.y = min.y + (max.y - min.y) * 0.5f;
        center.z = min.z + (max.z - min.z) * 0.5f;
        return center;
    }
};

//-----------------------------
// 衝突結果
//-----------------------------
struct Hit
{
    bool               isHit;       // 衝突しているかどうか
    DirectX::XMFLOAT3  normal;      // めり込みを解消するための法線方向
    float              penetration; // めり込み深度
};

//=============================
// 2D 当たり判定
//=============================

// 円と円の当たり判定
bool Collision_OverlapCircleCircle(const Circle& a, const Circle& b);

// 矩形（Box）同士の当たり判定
bool Collision_OverlapCircleBox(const Box& a, const Box& b);

//=============================
// 3D 当たり判定
//=============================

// AABB 同士の重なり判定（true/false のみ）
bool Collision_IsOverLapAABB(const AABB& a, const AABB& b);

// AABB 同士の当たり判定＋衝突法線の算出
Hit Collision_IsHitAABB(const AABB& a, const AABB& b);

//=============================
// デバッグ描画（初期化／終了）
//=============================

// デバッグ描画用の初期化（白テクスチャ・頂点バッファなど）
void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

// デバッグ描画用の終了処理
void Collision_DebugFinalize();

//=============================
// デバッグ描画（形ごと）
//=============================

// 円のデバッグ描画（デフォルトは黄色）
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 0.0f, 1.0f });

// 矩形（2D Box）のデバッグ描画（デフォルトは黄色）
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 0.0f, 1.0f });

// 3D AABB のデバッグ描画（枠線のみ）
void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4& color);

// キャラが「上に乗っている面」の Y を取得
// 戻り値：true = 取得成功
bool Collision_GetSupportY(
    const AABB& actor,
    const AABB* colliders,
    int colliderCount,
    float* outSupportY);


#endif // COLLISION_H