/*==============================================================================

   タイル壁Plane生成（床に向く壁・横連結・確定版） [TileWall.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

   ■このファイルがやること
   ・「壁タイル」かつ「隣が床タイル」の境界面だけを検出して WallPlane を生成する
   ・同じ向きで一直線に並ぶ壁面を横方向に連結して 1枚の WallPlane にする
   ・高さ方向は panelHeight ごとに分割して積む
   ・法線は常に「床側（内側）」を向く（床に向いて壁を設置する）

==============================================================================*/

#include "TileWall.h"

#include <algorithm>
#include <cmath>

// map.cpp 側と意味を合わせる
#define MAP_TILE_FLOOR 0
#define MAP_TILE_WALL  1
#define MAP_TILE_EMPTY 2


using namespace DirectX;


namespace
{
    //--------------------------------------------------------------------------
    // タイル判定系
    //--------------------------------------------------------------------------

    static bool IsWall(
        const std::vector<int>& tiles,
        int w, int h,
        int x, int y)
    {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return false;

        return tiles[y * w + x] == MAP_TILE_WALL;
    }




    static bool IsFloor(
        const std::vector<int>& tiles,
        int w, int h,
        int x, int y)
    {
        // 範囲外は床ではない
        if (x < 0 || x >= w || y < 0 || y >= h)
            return false;

        return tiles[y * w + x] == MAP_TILE_FLOOR;
    }

    //--------------------------------------------------------------------------
    // タイル座標 → ワールド座標（中心配置）
    //--------------------------------------------------------------------------

    static float TileX_ToWorld(float tx, int w, float cell, float originX)
    {
        return originX + (tx - w * 0.5f + 0.5f) * cell;
    }

    static float TileZ_ToWorld(float ty, int h, float cell, float originZ)
    {
        return originZ + (ty - h * 0.5f + 0.5f) * cell;
    }

    //--------------------------------------------------------------------------
    // run中心（start～end）の中心を float で保持（0.5ズレ防止）
    //--------------------------------------------------------------------------

    static float MidTile_KeepHalf(int start, int end)
    {
        return (static_cast<float>(start) + static_cast<float>(end)) * 0.5f;
    }
}

//==============================================================================
// 壁Plane生成メイン
//==============================================================================
void TileWall_BuildFromTiles(
    const std::vector<int>& tiles,
    int w,
    int h,
    float originX,
    float originZ,
    const TileWallParams& params,
    std::vector<WallPlane>& outPlanes)
{
    outPlanes.clear();

    //--------------------------------------------------------------------------
    // パラメータ展開
    //--------------------------------------------------------------------------
    const float cell = params.cellSize;
    const float half = cell * 0.5f;

    const float wallTotalH = std::max(params.wallHeight, 0.0f);
    const float panelH = std::max(params.panelHeight, 0.01f);
    const float uvUnit = std::max(params.uvUnitMeter, 0.0001f);

    const float wallBottomY = params.floorY;

    const int panelCount =
        static_cast<int>(std::ceil(wallTotalH / panelH));

    //--------------------------------------------------------------------------
    // 「高さ方向に積む」共通処理
    //--------------------------------------------------------------------------
    auto EmitPlaneStack =
        [&](const XMFLOAT3& normal,
            const XMFLOAT3& tangentU,
            float centerX,
            float centerZ,
            float widthW)
        {
            for (int i = 0; i < panelCount; ++i)
            {
                const float y0 = wallBottomY + panelH * i;

                float hNow = panelH;
                if (y0 + hNow > wallBottomY + wallTotalH)
                {
                    hNow = (wallBottomY + wallTotalH) - y0;
                    if (hNow <= 0.0001f)
                        break;
                }

                const float cy = y0 + hNow * 0.5f;

                WallPlane p{};
                p.normal = normal;
                p.tangentU = tangentU;

                // 境界面に配置：wallタイル中心 + normal * half
                // （normal は床側を向く＝内向き）
                p.center =
                {
                    centerX + normal.x * half,
                    cy,//描画バグを改善
                    centerZ + normal.z * half
                };

                p.width = widthW;
                p.height = hNow;

                p.uvRepeat =
                {
                    p.width / uvUnit,
                    p.height / uvUnit
                };

                outPlanes.push_back(p);
            }
        };

    //--------------------------------------------------------------------------
    // +X / -X 面（Z方向に連結）
    // ・「壁タイル」かつ「隣が床タイル」の面だけ生成
    //--------------------------------------------------------------------------

    for (int tx = 0; tx < w; ++tx)
    {
        int ty = 0;

        while (ty < h)
        {
            // +X 面（右が床）→ 法線は +X（床側を向く）
            if (IsWall(tiles, w, h, tx, ty) &&
                IsFloor(tiles, w, h, tx + 1, ty))
            {
                const int start = ty;

                while (ty < h &&
                    IsWall(tiles, w, h, tx, ty) &&
                    IsFloor(tiles, w, h, tx + 1, ty))
                {
                    ++ty;
                }

                const int end = ty - 1;
                const float widthW = (end - start + 1) * cell;

                const float midZ = MidTile_KeepHalf(start, end);
                const float cx = TileX_ToWorld(static_cast<float>(tx), w, cell, originX);
                const float cz = TileZ_ToWorld(midZ, h, cell, originZ);

                EmitPlaneStack(
                    { +1.0f, 0.0f, 0.0f },
                    { 0.0f, 0.0f, +1.0f },
                    cx, cz, widthW);;

                continue;
            }

            // -X 面（左が床）→ 法線は -X（床側を向く）
            if (IsWall(tiles, w, h, tx, ty) &&
                IsFloor(tiles, w, h, tx - 1, ty))
            {
                const int start = ty;

                while (ty < h &&
                    IsWall(tiles, w, h, tx, ty) &&
                    IsFloor(tiles, w, h, tx - 1, ty))
                {
                    ++ty;
                }

                const int end = ty - 1;
                const float widthW = (end - start + 1) * cell;

                const float midZ = MidTile_KeepHalf(start, end);
                const float cx = TileX_ToWorld(static_cast<float>(tx), w, cell, originX);
                const float cz = TileZ_ToWorld(midZ, h, cell, originZ);

                EmitPlaneStack(
                    { -1,0,0 },     // 法線（床側）
                    { 0,0,-1 },     // U方向（Z-）
                    cx, cz, widthW);

                continue;
            }

            ++ty;
        }
    }

    //--------------------------------------------------------------------------
    // +Z / -Z 面（X方向に連結）
    // ・「壁タイル」かつ「隣が床タイル」の面だけ生成
    //--------------------------------------------------------------------------

    for (int ty = 0; ty < h; ++ty)
    {
        int tx = 0;

        while (tx < w)
        {
            // +Z 面（上が床）→ 法線は +Z（床側を向く）
            if (IsWall(tiles, w, h, tx, ty) &&
                IsFloor(tiles, w, h, tx, ty + 1))
            {
                const int start = tx;

                while (tx < w &&
                    IsWall(tiles, w, h, tx, ty) &&
                    IsFloor(tiles, w, h, tx, ty + 1))
                {
                    ++tx;
                }

                const int end = tx - 1;
                const float widthW = (end - start + 1) * cell;

                const float midX = MidTile_KeepHalf(start, end);
                const float cx = TileX_ToWorld(midX, w, cell, originX);
                const float cz = TileZ_ToWorld(static_cast<float>(ty), h, cell, originZ);

                EmitPlaneStack(
                    { 0,0,+1 },     // 法線（床側）
                    { -1,0,0 },     // U方向（X-）
                    cx, cz, widthW);

                continue;
            }

            // -Z 面（下が床）→ 法線は -Z（床側を向く）
            if (IsWall(tiles, w, h, tx, ty) &&
                IsFloor(tiles, w, h, tx, ty - 1))
            {
                const int start = tx;

                while (tx < w &&
                    IsWall(tiles, w, h, tx, ty) &&
                    IsFloor(tiles, w, h, tx, ty - 1))
                {
                    ++tx;
                }

                const int end = tx - 1;
                const float widthW = (end - start + 1) * cell;

                const float midX = MidTile_KeepHalf(start, end);
                const float cx = TileX_ToWorld(midX, w, cell, originX);
                const float cz = TileZ_ToWorld(static_cast<float>(ty), h, cell, originZ);

                EmitPlaneStack(
                    { 0,0,-1 },     // 法線（床側）
                    { +1,0,0 },     // U方向（X+）
                    cx, cz, widthW);

                continue;
            }

            ++tx;
        }
    }
}
