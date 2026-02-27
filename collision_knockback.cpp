/*==============================================================================
   衝突ノックバックシステム [collision_knockback.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/16
--------------------------------------------------------------------------------
==============================================================================*/

#include "collision_knockback.h"
using namespace DirectX;

bool Collision_ApplyKnockback(
    XMVECTOR* playerPos,
    XMVECTOR* playerVel,
    XMVECTOR* enemyPos,
    XMVECTOR* enemyVel,
    const AABB& playerAABB,
    const AABB& enemyAABB,
    const KnockbackConfig& config)
{
    // AABB重なり判定
    const bool overlapX = (playerAABB.min.x <= enemyAABB.max.x && playerAABB.max.x >= enemyAABB.min.x);
    const bool overlapY = (playerAABB.min.y <= enemyAABB.max.y && playerAABB.max.y >= enemyAABB.min.y);
    const bool overlapZ = (playerAABB.min.z <= enemyAABB.max.z && playerAABB.max.z >= enemyAABB.min.z);

    if (!overlapX || !overlapY || !overlapZ)
        return false;  // 衝突なし

    // 中心点を計算
    const float playerCenterX = (playerAABB.min.x + playerAABB.max.x) * 0.5f;
    const float playerCenterZ = (playerAABB.min.z + playerAABB.max.z) * 0.5f;

    const float enemyCenterX = (enemyAABB.min.x + enemyAABB.max.x) * 0.5f;
    const float enemyCenterZ = (enemyAABB.min.z + enemyAABB.max.z) * 0.5f;

    // 押し戻し方向ベクトル（XZ平面のみ）
    XMVECTOR pushDir = XMVectorSet(
        playerCenterX - enemyCenterX,
        0.0f,
        playerCenterZ - enemyCenterZ,
        0.0f);

    const float lenSq = XMVectorGetX(XMVector3LengthSq(pushDir));

    if (lenSq < 0.0001f)
    {
        // 完全に重なっている場合はランダム方向へ
        pushDir = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        pushDir = XMVector3Normalize(pushDir);
    }

    // ノックバック速度を加算
    *playerVel += pushDir * config.playerForce;
    *enemyVel -= pushDir * config.enemyForce;  // 逆方向

    return true;
}