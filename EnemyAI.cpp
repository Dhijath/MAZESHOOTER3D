/*==============================================================================

   エネミーAI制御 [EnemyAI.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/17
--------------------------------------------------------------------------------

==============================================================================*/

#include "EnemyAI.h"
#include "Player.h"
#include "MapPatrolAI.h"
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

namespace
{
    //==========================================================================
    // AI設定パラメータ（全エネミー共通・速度は引数で個別指定）
    //==========================================================================
    float g_SightDistance = 5.0f;   // 視界距離（メートル）
    float g_RotationSpeed = 270.0f; // 回転速度（度/秒）
    float g_ArrivalDistance = 1.0f;   // 到達判定距離（メートル）
}

//==============================================================================
// AI更新処理（追跡 + 巡回）
//==============================================================================
void EnemyAI_Update(
    DirectX::XMFLOAT3* ioPosition,
    DirectX::XMFLOAT3* ioVelocity,
    DirectX::XMFLOAT3* ioFront,
    DirectX::XMFLOAT3* ioDestination,
    bool* ioWasChasing,
    float              dt,
    float              chaseSpeed,
    float              patrolSpeed)
{
    // プレイヤー位置取得
    const XMFLOAT3& playerPos = Player_GetPosition();

    // エネミー→プレイヤーのベクトル
    XMVECTOR vEnemyPos = XMLoadFloat3(ioPosition);
    XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);
    XMVECTOR vToPlayer = vPlayerPos - vEnemyPos;

    // 距離計算（XZ平面のみ・Yは無視）
    XMVECTOR vToPlayerXZ = XMVectorSetY(vToPlayer, 0.0f);
    float    distanceToPlayer = sqrtf(XMVectorGetX(XMVector3LengthSq(vToPlayerXZ)));

    //==========================================================================
    // モード判定：プレイヤーが視界内か？
    //==========================================================================

    // 追跡中は離脱距離を広めにとってチャタリングを防ぐ
    const float sightEnterDist = g_SightDistance;
    const float sightExitDist = g_SightDistance * 1.3f;
    const float effectiveDist = (*ioWasChasing) ? sightExitDist : sightEnterDist;

    bool isPlayerInSight =
        (distanceToPlayer <= effectiveDist) &&
        MapPatrolAI_HasLineOfSight(*ioPosition, playerPos);

    XMVECTOR vTargetDir;
    float    moveSpeed;

    if (isPlayerInSight)
    {
        //======================================================================
        // 追跡モード：プレイヤー方向へ
        //======================================================================
        *ioWasChasing = true;
        vTargetDir = XMVector3Normalize(vToPlayerXZ);
        moveSpeed = chaseSpeed; // 引数で受け取った速度を使用する
    }
    else
    {
        //======================================================================
        // 巡回モード：目的地へ向かう
        //======================================================================

        // 追跡から巡回に切り替わった瞬間に目的地を再取得する
        if (*ioWasChasing)
        {
            *ioDestination = MapPatrolAI_GetReachableDestination(*ioPosition);
            *ioWasChasing = false;
        }

        // エネミー→目的地のベクトル
        XMVECTOR vDest = XMLoadFloat3(ioDestination);
        XMVECTOR vToDestXZ = XMVectorSetY(vDest - vEnemyPos, 0.0f);
        float    distToDest = sqrtf(XMVectorGetX(XMVector3LengthSq(vToDestXZ)));

        // 目的地到達チェック
        if (distToDest < g_ArrivalDistance)
        {
            // 新しい目的地をランダム取得する
            *ioDestination = MapPatrolAI_GetReachableDestination(*ioPosition);
            vDest = XMLoadFloat3(ioDestination);
            vToDestXZ = XMVectorSetY(vDest - vEnemyPos, 0.0f);
        }

        vTargetDir = XMVector3Normalize(vToDestXZ);
        moveSpeed = patrolSpeed; // 引数で受け取った速度を使用する
    }

    //==========================================================================
    // 向き変更（徐々に回転）
    //==========================================================================

    XMVECTOR vCurrentFront = XMVector3Normalize(XMVectorSetY(XMLoadFloat3(ioFront), 0.0f));

    // 目標向きとのなす角（ラジアン）
    float dot = std::clamp(XMVectorGetX(XMVector3Dot(vCurrentFront, vTargetDir)), -1.0f, 1.0f);
    float angleRad = acosf(dot);

    // 今フレームで回せる最大角度（ラジアン）
    float maxRotRad = XMConvertToRadians(g_RotationSpeed) * dt;

    XMVECTOR vNewFront;

    if (angleRad <= maxRotRad)
    {
        // 一気に目標方向へ向く
        vNewFront = vTargetDir;
    }
    else
    {
        // 左右どちらに回すか判定して徐々に回転する
        float     crossY = XMVectorGetY(XMVector3Cross(vCurrentFront, vTargetDir));
        XMMATRIX  rotMtx = XMMatrixRotationY(crossY >= 0.0f ? maxRotRad : -maxRotRad);
        vNewFront = XMVector3Normalize(XMVector3TransformNormal(vCurrentFront, rotMtx));
    }

    XMStoreFloat3(ioFront, vNewFront);

    //==========================================================================
    // 移動：向いている方向へ加速
    //==========================================================================
    XMVECTOR vVelocity = XMLoadFloat3(ioVelocity);
    XMVECTOR vTargetVelocity = vNewFront * moveSpeed;

    // 現在のXZ速度を目標速度へ補間する（慣性を持たせつつ壁貼り付きを防ぐ）
    const float lerpSpeed = 10.0f;
    float       lerpT = std::min(1.0f, lerpSpeed * dt);

    float newVx = XMVectorGetX(vVelocity) + (XMVectorGetX(vTargetVelocity) - XMVectorGetX(vVelocity)) * lerpT;
    float newVz = XMVectorGetZ(vVelocity) + (XMVectorGetZ(vTargetVelocity) - XMVectorGetZ(vVelocity)) * lerpT;

    vVelocity = XMVectorSetX(vVelocity, newVx);
    vVelocity = XMVectorSetZ(vVelocity, newVz);

    XMStoreFloat3(ioVelocity, vVelocity);
}

//==============================================================================
// 視界距離設定
//==============================================================================
void EnemyAI_SetSightDistance(float distance)
{
    g_SightDistance = std::max(0.1f, distance);
}

//==============================================================================
// 回転速度設定
//==============================================================================
void EnemyAI_SetRotationSpeed(float degreesPerSecond)
{
    g_RotationSpeed = std::max(1.0f, degreesPerSecond);
}

//==============================================================================
// 到達判定距離設定
//==============================================================================
void EnemyAI_SetArrivalDistance(float distance)
{
    g_ArrivalDistance = std::max(0.1f, distance);
}