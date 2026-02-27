/*==============================================================================

   スプライトアニメ管理 [Sprite_Anime.h]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SPRITE_ANIME_H
#define SPRITE_ANIME_H

#include <DirectXMath.h>

// スプライトアニメシステム初期化／終了
void SpriteAnim_Initialize();
void SpriteAnim_Finalize(void);

// アニメ全体の更新・描画
void SpriteAnim_Update(double elapsed_time);
void SpriteAnim_Draw(int playid, float x, float y, float dw, float dh);

// ビルボード版アニメ描画（3D 空間）
void BillboardAnim_Draw(
    int playid,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }
);

// アニメパターン登録
//  textrueId          : 使用するテクスチャ ID
//  pattern_max        : パターン総数
//  second_per_pattern : 1コマあたりの秒数
//  pattern_size       : 1パターンのサイズ（幅・高さ）
//  start_position     : テクスチャ上の開始位置
//  isLooped           : ループ再生するか
//  pattern_col        : 1行あたりのパターン数（横方向）
int SpriteAnim_PatternRegister(
    int textrueId,
    int pattern_max,
    double second_per_pattern,
    const DirectX::XMUINT2& pattern_size,
    const DirectX::XMUINT2& start_position,
    bool isLooped = true,
    int pattern_col = 1
);

// パターンを使う「プレイヤー」を作成（再生インスタンス）
int SpriteAnim_CreatePlayer(int anime_pattern_id);

// 指定プレイヤーが停止状態かどうか
bool SpriteAnim_IsStopped(int index);

// プレイヤー破棄（スロットを開放）
void SpriteAnim_DestroyPlayer(int index);

#endif // SPRITE_ANIME_H
