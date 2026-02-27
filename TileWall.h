/*==============================================================================

   タイル壁Plane生成 [TileWall.h]
                                                         Author : 51106
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------
   ■このファイルがやること
   ・タイル（床/壁）から「見える壁面」だけを抽出して WallPlane を生成する
   ・WallPlane は向き（法線）を保持し、UVの向きが固定で揃うよう tangentU を決める
   ・壁の高さは「固定パネル」を積む方式で頂点数を抑える
   ・UVは伸縮させず、m単位で繰り返す（uRepeat/vRepeat）

==============================================================================*/

#ifndef TILE_WALL_H
#define TILE_WALL_H

#include <DirectXMath.h>
#include <vector>
#include <cstdint>

struct WallPlane
{
    DirectX::XMFLOAT3 center;     // 面の中心
    DirectX::XMFLOAT3 normal;     // ±X / ±Z
    DirectX::XMFLOAT3 tangentU;   // U方向（壁に沿う方向）※向き固定の要
    float width;                  // 面の長さ（m）
    float height;                 // 面の高さ（m）＝パネル高さ（最後だけ余りOK）
    DirectX::XMFLOAT2 uvRepeat;   // (uRepeat, vRepeat)
};

// タイル種別（map.cpp と合わせる）
enum TileType : int
{
    TILE_FLOOR = 0,
    TILE_WALL = 1,
};

struct TileWallParams
{
    float cellSize = 1.0f;  // 1タイル=1m
    float floorY = 0.5f;  // 床中心（既存に合わせる）
    float wallHeight = 10.0f; // 壁総高さ
    float panelHeight = 2.0f;  // 1パネルの高さ
    float uvUnitMeter = 1.0f;  // 1mあたり1回ループ（u/v共通）
};

// tiles は y*w + x の一次元
// originX/originZ と w/h は、既存 TileCoordToWorldCenter と同じ基準
void TileWall_BuildFromTiles(
    const std::vector<int>& tiles,
    int w, int h,
    float originX, float originZ,
    const TileWallParams& params,
    std::vector<WallPlane>& outPlanes);

#endif // TILE_WALL_H

