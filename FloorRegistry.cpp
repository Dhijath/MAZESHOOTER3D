/*==============================================================================

   床レジストリ管理 [FloorRegistry.cpp]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------
   ・(x,z) がAABBの水平範囲内にある床を候補として扱う
   ・topY = aabb.max.y を「床上面」として使用

   supportY_local:
     footY + EPS 以上を除外し、footY以下で最も高い topY
   supportY_base:
     同一点に存在する床の topY の最小値（地面基準）

==============================================================================*/
#include "FloorRegistry.h"
#include <cfloat>
#include <algorithm>

namespace
{
    std::vector<AABB> g_Floors;

    inline bool ContainsXZ(const AABB& a, float x, float z)
    {
        return (x >= a.min.x && x <= a.max.x &&
            z >= a.min.z && z <= a.max.z);
    }
}

namespace FloorRegistry
{
    void Clear()
    {
        g_Floors.clear();
    }

    void Add(const AABB& aabb)
    {
        g_Floors.push_back(aabb);
    }

    size_t Count()
    {
        return g_Floors.size();
    }

    bool GetSupportY_Local(float x, float z, float footY, float* outSupportY)
    {
        if (!outSupportY) return false;

        constexpr float EPS = 0.001f;

        bool found = false;
        float best = -FLT_MAX;

        for (const auto& a : g_Floors)
        {
            if (!ContainsXZ(a, x, z)) continue;

            const float topY = a.max.y;

            if (topY > footY + EPS) continue;

            if (!found || topY > best)
            {
                best = topY;
                found = true;
            }
        }

        if (found) *outSupportY = best;
        return found;
    }

    bool GetSupportY_LocalAndBase(
        float x, float z, float footY,
        float* outLocalY,
        float* outBaseY)
    {
        if (!outLocalY || !outBaseY) return false;

        constexpr float EPS = 0.001f;

        bool any = false;

        float baseY = +FLT_MAX;

        bool hasLocal = false;
        float localY = -FLT_MAX;

        for (const auto& a : g_Floors)
        {
            if (!ContainsXZ(a, x, z)) continue;

            any = true;
            const float topY = a.max.y;

            baseY = std::min(baseY, topY);

            if (topY <= footY + EPS)
            {
                if (!hasLocal || topY > localY)
                {
                    localY = topY;
                    hasLocal = true;
                }
            }
        }

        if (!any) return false;

        *outBaseY = baseY;
        *outLocalY = hasLocal ? localY : baseY;

        return true;
    }
}
