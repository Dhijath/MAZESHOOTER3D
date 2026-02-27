/*==============================================================================
   衝突ノックバックシステム [collision_knockback.h]
                                                         Author : 51106
                                                         Date   : 2026/01/16
--------------------------------------------------------------------------------
   ■役割
   ・エネミー ↔ プレイヤーの相互ノックバック処理
   ・弾の判定は対象外（Enemy.cpp側で個別処理）
==============================================================================*/

#pragma once
#include <DirectXMath.h>
#include "collision.h"

//==============================================================================
// ノックバック設定
//==============================================================================
struct KnockbackConfig
{
    float playerForce;  // プレイヤーが受ける力
    float enemyForce;   // エネミーが受ける力

    static KnockbackConfig Default()
    {
        return { 5.0f, 3.0f };  // プレイヤー5.0, エネミー3.0
    }
};

//==============================================================================
// プレイヤー ↔ エネミー衝突判定＆ノックバック
// 
// ■引数
// ・playerPos/Vel : プレイヤーの位置・速度（更新される）
// ・enemyPos/Vel  : エネミーの位置・速度（更新される）
// ・playerAABB    : プレイヤーのAABB
// ・enemyAABB     : エネミーのAABB
// ・config        : ノックバック設定
// 
// ■戻り値
// ・true: 衝突してノックバック発生
// ・false: 衝突なし
//==============================================================================
bool Collision_ApplyKnockback(
    DirectX::XMVECTOR* playerPos,
    DirectX::XMVECTOR* playerVel,
    DirectX::XMVECTOR* enemyPos,
    DirectX::XMVECTOR* enemyVel,
    const AABB& playerAABB,
    const AABB& enemyAABB,
    const KnockbackConfig& config = KnockbackConfig::Default());
