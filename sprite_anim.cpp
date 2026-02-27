/*==============================================================================

   スプライトアニメ管理 [Sprite_Anim.cpp]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------

==============================================================================*/

#include "sprite_Anim.h"
#include "sprite.h"
#include "texture.h"
#include <DirectXMath.h>
#include "billboard.h"

using namespace DirectX;

//==============================================================================
// アニメーションパターン定義
// ・テクスチャ上のどこから
// ・何コマ
// ・何秒ごとに進むか
// ・ループするか
// をまとめた「アニメの設計図」
//==============================================================================
struct AnimPatternData
{
    int     m_TextureId = -1;          // テクスチャID（未使用時は -1）
    int     m_PatternMax = 0;           // パターン数（コマ総数）
    int     m_PatternCol = 1;           // パターンの列数（横方向のコマ数）
    XMUINT2 m_StartPosition{ 0, 0 };      // アニメーションのスタート座標（テクスチャ上）
    XMUINT2 m_PatternSize{ 0, 0 };      // 1 パターンのサイズ（幅・高さ）
    bool    m_IsLooped = true;        // ループするかどうか
    double  m_second_per_pattern = 0.1;      // 1コマあたりの秒数（デフォルト 0.1 秒）
};

//==============================================================================
// アニメーション再生状態
// ・どのパターンを使っているか
// ・今どのコマか
// ・どれだけ時間が経ったか
// ・停止中かどうか
// をまとめた「再生プレイヤー」
//==============================================================================
struct AnimPlayData
{
    int    m_PatternId = -1;      // 使用中のアニメパターンID（未使用時は -1）
    int    m_PatternNum = 0;       // 現在再生中のパターン番号（0 ～）
    double m_AccumulatedTime = 0.0;     // 累積時間
    bool   m_isStopped = false;   // true = 再生停止中（最後のコマで止まっている等）
};

// パターン／プレイヤーの最大数
static constexpr int ANIM_PATTERN_MAX = 128;
static constexpr int ANIM_PLAY_MAX = 256;

// パターンとプレイヤー実体
static AnimPatternData g_AnimPattern[ANIM_PATTERN_MAX]; // アニメーションパターンデータ
static AnimPlayData    g_AnimPlayData[ANIM_PLAY_MAX];   // アニメーション再生データ

//==============================================================================
// 初期化
// ・パターン／プレイヤーをすべて未使用状態に
//==============================================================================
void SpriteAnim_Initialize()
{
    // パターンスロット初期化
    for (AnimPatternData& data : g_AnimPattern)
    {
        data.m_TextureId = -1; // 未使用マーク
    }

    // プレイヤースロット初期化
    for (AnimPlayData& data : g_AnimPlayData)
    {
        data.m_PatternId = -1;    // 未使用マーク
        data.m_isStopped = false; // 停止フラグ初期化
        data.m_PatternNum = 0;
        data.m_AccumulatedTime = 0.0;
    }
}

//==============================================================================
// 終了処理（今のところやること無し）
//==============================================================================
void SpriteAnim_Finalize(void)
{
}

//==============================================================================
// 更新処理
// ・全プレイヤーに対して経過時間を反映
// ・しきい値を超えたら次のコマへ進める
// ・ループしないものは最後のコマで停止
//==============================================================================
void SpriteAnim_Update(double elapsed_time)
{
    for (int i = 0; i < ANIM_PLAY_MAX; i++)
    {
        // 未使用プレイヤーはスキップ
        if (g_AnimPlayData[i].m_PatternId < 0) continue;

        AnimPatternData* pAnmPtrnData =
            &g_AnimPattern[g_AnimPlayData[i].m_PatternId];

        // 1コマ分の時間を超えていたら次のコマへ
        if (g_AnimPlayData[i].m_AccumulatedTime >= pAnmPtrnData->m_second_per_pattern)
        {
            g_AnimPlayData[i].m_PatternNum++; // パターン番号を進める

            // 最後のコマを超えたか？
            if (g_AnimPlayData[i].m_PatternNum >= pAnmPtrnData->m_PatternMax)
            {
                if (pAnmPtrnData->m_IsLooped)
                {
                    // ループする場合は最初に戻る
                    g_AnimPlayData[i].m_PatternNum = 0;
                }
                else
                {
                    // ループしない場合は最後のコマで固定して停止
                    g_AnimPlayData[i].m_PatternNum =
                        pAnmPtrnData->m_PatternMax - 1;
                    g_AnimPlayData[i].m_isStopped = true;
                }
            }

            // 「1コマ分の時間」を引いて残りを次フレームに回す
            g_AnimPlayData[i].m_AccumulatedTime -= pAnmPtrnData->m_second_per_pattern;
        }

        // 累積時間を更新
        g_AnimPlayData[i].m_AccumulatedTime += elapsed_time;
    }
}

//==============================================================================
// 2D スプライトとして描画
// playid : 再生プレイヤー ID
// x, y   : 描画位置
// dw, dh : 描画サイズ
//==============================================================================
void SpriteAnim_Draw(int playid, float x, float y, float dw, float dh)
{
    int anm_ptrn_id = g_AnimPlayData[playid].m_PatternId;
    AnimPatternData* pAnmPtrnData = &g_AnimPattern[anm_ptrn_id];

    // 今再生中のコマ番号
    int frame = g_AnimPlayData[playid].m_PatternNum;

    // パターンの列数で割り算して X,Y 方向のインデックスを求める
    int col = frame % pAnmPtrnData->m_PatternCol;
    int row = frame / pAnmPtrnData->m_PatternCol;

    // テクスチャ上の切り出し位置（ピクセル単位）
    int src_x = pAnmPtrnData->m_StartPosition.x +
        pAnmPtrnData->m_PatternSize.x * col;
    int src_y = pAnmPtrnData->m_StartPosition.y +
        pAnmPtrnData->m_PatternSize.y * row;

    // 実際にスプライト描画
    Sprite_Draw(
        pAnmPtrnData->m_TextureId,
        x, y, dw, dh,
        src_x,
        src_y,
        pAnmPtrnData->m_PatternSize.x,
        pAnmPtrnData->m_PatternSize.y
    );
}

//==============================================================================
// ビルボードとして 3D 空間にアニメを描画
//==============================================================================
void BillboardAnim_Draw(
    int playid,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT2& pivot)
{
    int anm_ptrn_id = g_AnimPlayData[playid].m_PatternId;
    AnimPatternData* pAnmPtrnData = &g_AnimPattern[anm_ptrn_id];

    int frame = g_AnimPlayData[playid].m_PatternNum;
    int col = frame % pAnmPtrnData->m_PatternCol;
    int row = frame / pAnmPtrnData->m_PatternCol;

    // テクスチャ上の切り出し矩形（x, y, w, h）
    XMFLOAT4 tex_cut =
    {
        static_cast<float>(pAnmPtrnData->m_StartPosition.x +
                           pAnmPtrnData->m_PatternSize.x * col),
        static_cast<float>(pAnmPtrnData->m_StartPosition.y +
                           pAnmPtrnData->m_PatternSize.y * row),
        static_cast<float>(pAnmPtrnData->m_PatternSize.x),
        static_cast<float>(pAnmPtrnData->m_PatternSize.y)
    };

    // ビルボード描画
    Billboard_Draw(
        pAnmPtrnData->m_TextureId,
        position,
        scale,
        tex_cut,
        pivot
    );
}

//==============================================================================
// アニメーションパターンを登録
// 空いているスロットに設定して、そのインデックスを返す
//==============================================================================
int SpriteAnim_PatternRegister(
    int textrueId,
    int pattern_max,
    double second_per_pattern,
    const DirectX::XMUINT2& pattern_size,
    const DirectX::XMUINT2& start_position,
    bool isLooped,
    int pattern_col)
{
    for (int i = 0; i < ANIM_PATTERN_MAX; i++)
    {
        // すでに使用中のスロットはスキップ
        if (g_AnimPattern[i].m_TextureId >= 0) continue;

        g_AnimPattern[i].m_TextureId = textrueId;
        g_AnimPattern[i].m_PatternMax = pattern_max;
        g_AnimPattern[i].m_second_per_pattern = second_per_pattern;
        g_AnimPattern[i].m_PatternCol = pattern_col;
        g_AnimPattern[i].m_StartPosition = start_position;
        g_AnimPattern[i].m_PatternSize = pattern_size;
        g_AnimPattern[i].m_IsLooped = isLooped;

        // 登録したパターン ID を返す
        return i;
    }

    // 空きがなかった
    return -1;
}

//==============================================================================
// アニメ再生プレイヤーを生成
// 指定パターンを再生するプレイヤーを作り、そのインデックスを返す
//==============================================================================
int SpriteAnim_CreatePlayer(int Anim_pattern_id)
{
    for (int i = 0; i < ANIM_PLAY_MAX; i++)
    {
        // すでに使用中のプレイヤーはスキップ
        if (g_AnimPlayData[i].m_PatternId >= 0) continue;

        g_AnimPlayData[i].m_PatternId = Anim_pattern_id;
        g_AnimPlayData[i].m_PatternNum = 0;
        g_AnimPlayData[i].m_AccumulatedTime = 0.0;
        g_AnimPlayData[i].m_isStopped = false;

        // このプレイヤーIDを返す
        return i;
    }

    // 空きがなかった
    return -1;
}

//==============================================================================
// 指定プレイヤーが停止しているかどうか
//==============================================================================
bool SpriteAnim_IsStopped(int index)
{
    return g_AnimPlayData[index].m_isStopped;
}

//==============================================================================
// プレイヤーの破棄（スロット解放）
//==============================================================================
void SpriteAnim_DestroyPlayer(int index)
{
    // パターン ID を無効化するだけで「未使用」扱い
    g_AnimPlayData[index].m_PatternId = -1;
}
