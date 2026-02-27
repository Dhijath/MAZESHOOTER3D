/*==============================================================================

   床レジストリ管理 [FloorRegistry.h]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------
   ・床AABBを毎フレーム登録し、(x,z)直下の支持面Yを問い合わせる
   ・AABB 型は collision.h のものをそのまま使う（重複定義しない）

==============================================================================*/
#pragma once

#include <vector>
#include "collision.h" //  AABB はここから使う

namespace FloorRegistry
{
    void Clear();
    void Add(const AABB& aabb);

    bool GetSupportY_Local(float x, float z, float footY, float* outSupportY);

    bool GetSupportY_LocalAndBase(
        float x, float z, float footY,
        float* outLocalY,
        float* outBaseY);

    size_t Count();
}

