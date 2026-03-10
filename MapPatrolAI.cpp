/*==============================================================================

   マップ巡回AI [MapPatrolAI.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/17
--------------------------------------------------------------------------------
==============================================================================*/

#include "MapPatrolAI.h"
#include "map.h"
#include <random>

namespace
{
    //==========================================================================
    // 床座標リスト（マップ生成時に設定）
    //==========================================================================
    std::vector<DirectX::XMFLOAT3> g_FloorPositions;

    //==========================================================================
    // 乱数生成器（目的地選択用）
    //==========================================================================
    std::mt19937 g_Rng;
}

//==============================================================================
// 床座標リスト初期化
//==============================================================================
void MapPatrolAI_Initialize(const std::vector<DirectX::XMFLOAT3>& floorPositions)
{
    g_FloorPositions = floorPositions;

    // 乱数生成器をシード初期化（時刻ベース）
    g_Rng.seed(static_cast<unsigned int>(time(nullptr)));
}

//==============================================================================
// ランダムな巡回目的地を取得
//==============================================================================
DirectX::XMFLOAT3 MapPatrolAI_GetRandomDestination()
{
    if (g_FloorPositions.empty())
    {
        // 床がない場合はデフォルト位置
        return { 0.0f, 1.0f, 0.0f };
    }

    // ランダムにインデックスを選択
    std::uniform_int_distribution<size_t> dist(0, g_FloorPositions.size() - 1);
    size_t randomIndex = dist(g_Rng);

    return g_FloorPositions[randomIndex];
}

//==============================================================================
// 床座標リストのクリア
//==============================================================================
void MapPatrolAI_Clear()
{
    g_FloorPositions.clear();
}

//==============================================================================
// 視線遮蔽判定
//
// ■役割
// ・from → to にレイを飛ばし、壁AABBに当たるか確認する
// ・Map_RaycastWalls に委譲するだけのラッパー
//
// ■引数
// ・from : 視線の始点（敵の位置）
// ・to   : 視線の終点（プレイヤーの位置）
//
// ■戻り値
// ・true  : 壁がなく見えている
// ・false : 壁に遮られて見えていない
//==============================================================================
bool MapPatrolAI_HasLineOfSight(
    const DirectX::XMFLOAT3& from,
    const DirectX::XMFLOAT3& to)
{
    // Y を壁の中央付近に固定してレイキャストする
    const float wallCenterY = 1.5f;

    DirectX::XMFLOAT3 fromFlat = { from.x, wallCenterY, from.z };
    DirectX::XMFLOAT3 toFlat = { to.x,   wallCenterY, to.z };

    DirectX::XMFLOAT3 hitPos{};
    return !Map_RaycastWalls(fromFlat, toFlat, &hitPos);
}
//==============================================================================
// 到達可能な巡回目的地を取得
//
// ■役割
// ・ランダムに目的地を選び from から直線で行けるか確認する
// ・壁に遮られた座標は引き直す（maxTry 回まで）
// ・maxTry 回以内に見つからなければ完全ランダムで返す
//
// ■引数
// ・from   : 現在地（敵の位置）
// ・maxTry : 最大試行回数
//
// ■戻り値
// ・到達可能な床座標
//==============================================================================
DirectX::XMFLOAT3 MapPatrolAI_GetReachableDestination(
    const DirectX::XMFLOAT3& from,
    int maxTry)
{
    if (g_FloorPositions.empty())
    {
        return { 0.0f, 1.0f, 0.0f };
    }

    std::uniform_int_distribution<size_t> dist(0, g_FloorPositions.size() - 1);

    for (int i = 0; i < maxTry; ++i)
    {
        const DirectX::XMFLOAT3& candidate = g_FloorPositions[dist(g_Rng)];

        const float wallCenterY = 1.5f;
        DirectX::XMFLOAT3 fromFlat = { from.x,      wallCenterY, from.z };
        DirectX::XMFLOAT3 candidateFlat = { candidate.x, wallCenterY, candidate.z };

        DirectX::XMFLOAT3 hitPos{};
        if (!Map_RaycastWalls(fromFlat, candidateFlat, &hitPos))
        {
            return candidate;
        }
    }

    // maxTry 回以内に見つからなければ完全ランダムで返す
    return g_FloorPositions[dist(g_Rng)];
}

DirectX::XMFLOAT3 MapPatrolAI_GetNearbyDestination(
    const DirectX::XMFLOAT3& from,
    float searchRadius,
    int maxTry)
{
    if (g_FloorPositions.empty())
    {
        return { 0.0f, 1.0f, 0.0f };
    }

    // 半径以内の候補を収集する
    std::vector<const DirectX::XMFLOAT3*> candidates;
    candidates.reserve(32);

    const float radiusSq = searchRadius * searchRadius;

    for (const DirectX::XMFLOAT3& pos : g_FloorPositions)
    {
        const float dx = pos.x - from.x;
        const float dz = pos.z - from.z;
        const float distSq = dx * dx + dz * dz;

        if (distSq <= radiusSq)
        {
            candidates.push_back(&pos);
        }
    }

    // 候補がなければ通常の到達可能目的地にフォールバック
    if (candidates.empty())
    {
        return MapPatrolAI_GetReachableDestination(from, maxTry);
    }

    // 候補をシャッフルして視線が通るものを返す
    std::shuffle(candidates.begin(), candidates.end(), g_Rng);

    const float wallCenterY = 1.5f;
    DirectX::XMFLOAT3 fromFlat = { from.x, wallCenterY, from.z };

    const int tryCount = std::min(maxTry, static_cast<int>(candidates.size()));

    for (int i = 0; i < tryCount; ++i)
    {
        const DirectX::XMFLOAT3& candidate = *candidates[i];
        DirectX::XMFLOAT3 candidateFlat = { candidate.x, wallCenterY, candidate.z };

        DirectX::XMFLOAT3 hitPos{};
        if (!Map_RaycastWalls(fromFlat, candidateFlat, &hitPos))
        {
            return candidate;
        }
    }

    // 視線が通るものが見つからなければフォールバック
    return MapPatrolAI_GetReachableDestination(from, maxTry);
}
