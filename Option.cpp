/*==============================================================================
   オプション [Option.cpp]
   Author : 51106
   Date   : 2025/02/01
--------------------------------------------------------------------------------
   音量調整などオプション画面の処理
   - ← → または 十字キー左右 で音量を増減
   - Enter または Aボタン で終了（タイトルへ戻る）
   - 音量設定は保存され、次回起動時も維持される
==============================================================================*/

#include "Option.h"                                      // オプション公開API
#include "texture.h"                                     // テクスチャ読み込み
#include "sprite.h"                                      // スプライト描画
#include "key_logger.h"                                  // キーボード入力
#include "pad_logger.h"                                  // ゲームパッド入力
#include "Audio.h"                                       // SetMasterVolume を使用
#include "direct3d.h"                                    // バックバッファサイズ取得
#include <DirectXMath.h>                                 // XMFLOAT4など
#include <algorithm>                                     // std::min / std::max

using namespace DirectX;                                 // DirectXMath名前空間

namespace                                                // ファイル内限定
{                                                        // ここから
    //==========================================================================
    // グローバルリソース
    //==========================================================================
    int g_OptionBgTex = -1;                              // 背景テクスチャ
    int g_WhiteTex = -1;                                 // 白テクスチャ（バー表示用）
    int g_TextControlsOptionTex = -1;                    //オプション操作説明テクスチャ    
    int g_TextControlsPlayerTex = -1;                    //ゲーム内操作説明テクスチャ    
    int g_TextVolumeLabelTex = -1;                       // "Volume"ラベルテクスチャ

    //==========================================================================
    // 音量管理
    //==========================================================================
    float g_Volume = 1.0f;                               // 音量値（0.0 ～ 1.0）
    bool g_VolumeInitialized = false;                    // 音量が初期化済みか

    //==========================================================================
    // 終了フラグ（ワンショット）
    //==========================================================================
    bool g_End = false;                                  // 終了フラグ
}                                                        // namespace終わり

//==============================================================================
// 初期化
//==============================================================================

void Option_Initialize()                                 // 初期化
{                                                        // 開始
    g_OptionBgTex = Texture_Load(L"resource/texture/titleBg.png"); // 背景読み込み
    g_WhiteTex = Texture_Load(L"resource/texture/white.png"); // 白テクスチャ読み込み
    g_TextControlsOptionTex = Texture_Load(L"resource/texture/text_controls_option.png"); // オプション操作説明読み込み
    g_TextControlsPlayerTex = Texture_Load(L"resource/texture/text_controls_player.png"); // プレイヤー操作説明読み込み
    g_TextVolumeLabelTex = Texture_Load(L"resource/texture/text_volume_label.png"); // Volumeラベル読み込み

    // 音量は初回のみ初期化（2回目以降は前回の値を維持）
    if (!g_VolumeInitialized)                            // 初回起動
    {                                                    // 開始
        g_Volume = 0.50f;                                // デフォルト音量
        g_VolumeInitialized = true;                      // 初期化済みフラグON
        SetMasterVolume(g_Volume);                       // 音量を反映
    }                                                    // 終了
    else                                                 // 2回目以降
    {                                                    // 開始
        SetMasterVolume(g_Volume);                       // 保存された音量を反映
    }                                                    // 終了

    g_End = false;                                       // 終了フラグクリア
}                                                            // 終了

//==============================================================================
// 終了処理
//==============================================================================
void Option_Finalize()                                   // 終了処理
{                                                        // 開始
    // 特にリソース解放は不要（Texture_Finalize でまとめて解放）
}                                                        // 終了

//==============================================================================
// 更新処理
//==============================================================================
void Option_Update(double elapsed_time)                  // 毎フレーム更新
{                                                        // 開始
    (void)elapsed_time;                                  // 未使用警告抑止

    // ← または 十字キー左 で音量を下げる
    if (KeyLogger_IsTrigger(KK_LEFT) || PadLogger_IsTrigger(PAD_DPAD_LEFT))
    {                                                    // 開始
        g_Volume = std::max(0.0f, g_Volume - 0.1f);      // 0.1刻みで減少（下限0.0）
        SetMasterVolume(g_Volume);                       // 音量を反映
    }                                                    // 終了

    // → または 十字キー右 で音量を上げる
    if (KeyLogger_IsTrigger(KK_RIGHT) || PadLogger_IsTrigger(PAD_DPAD_RIGHT))
    {                                                    // 開始
        g_Volume = std::min(1.0f, g_Volume + 0.1f);      // 0.1刻みで増加（上限1.0）
        SetMasterVolume(g_Volume);                       // 音量を反映
    }                                                    // 終了

    // Enter または Aボタン で終了
    if (KeyLogger_IsTrigger(KK_ENTER) || PadLogger_IsTrigger(PAD_A))
    {                                                    // 開始
        g_End = true;                                    // 終了フラグON
    }                                                    // 終了
}                                                        // 終了
//==============================================================================
// 描画処理
//==============================================================================
void Option_Draw()                                       // 描画処理
{                                                        // 開始
    const int sw = Direct3D_GetBackBufferWidth();       // 画面幅取得
    const int sh = Direct3D_GetBackBufferHeight();      // 画面高さ取得

    //==========================================================================
    // 背景を描画（画面全体にフィット）
    //==========================================================================
    if (g_OptionBgTex >= 0)                              // 背景テクスチャ有効
    {                                                    // 開始
        const float tw = (float)Texture_Width(g_OptionBgTex);   // 背景画像の幅
        const float th = (float)Texture_Height(g_OptionBgTex);  // 背景画像の高さ
        const float sx = (float)sw / std::max(1.0f, tw); // 幅の拡大率
        const float sy = (float)sh / std::max(1.0f, th); // 高さの拡大率
        Sprite_Draw(g_OptionBgTex, 0, 0, tw * sx, th * sy, XMFLOAT4(1, 1, 1, 1)); // 描画
    }                                                    // 終了

    //==========================================================================
    // オプション操作説明（左上）
    //==========================================================================
    if (g_TextControlsOptionTex >= 0)                    // オプション操作説明テクスチャ有効
    {                                                    // 開始
        const float w = (float)Texture_Width(g_TextControlsOptionTex);   // テクスチャ幅
        const float h = (float)Texture_Height(g_TextControlsOptionTex);  // テクスチャ高さ
        const float x = 30.0f;                           // 左端から30px
        const float y = 30.0f;                           // 上端から30px

        Sprite_Draw(g_TextControlsOptionTex, x, y, w, h, XMFLOAT4(1, 1, 1, 1)); // 描画
    }                                                    // 終了

    //==========================================================================
    // プレイヤー操作説明（右上）
    //==========================================================================
    if (g_TextControlsPlayerTex >= 0)                    // プレイヤー操作説明テクスチャ有効
    {                                                    // 開始
        const float w = (float)Texture_Width(g_TextControlsPlayerTex);   // テクスチャ幅
        const float h = (float)Texture_Height(g_TextControlsPlayerTex);  // テクスチャ高さ
        const float x = sw - w - 30.0f;                  // 右端から30px（左揃え）
        const float y = 30.0f;                           // 上端から30px

        Sprite_Draw(g_TextControlsPlayerTex, x, y, w, h, XMFLOAT4(1, 1, 1, 1)); // 描画
    }                                                    // 終了

    //==========================================================================
    // 音量バー（中央）
    //==========================================================================
    if (g_WhiteTex >= 0)                                 // 白テクスチャ有効
    {                                                    // 開始
        const float barMaxW = 400.0f;                    // バーの最大幅
        const float barH = 32.0f;                        // バーの高さ
        const float barX = (sw - barMaxW) * 0.5f;        // バーのX座標（中央揃え）
        const float barY = sh * 0.55f;                   // バーのY座標

        // 背景バー（グレー）
        Sprite_Draw(g_WhiteTex, barX, barY, barMaxW, barH, XMFLOAT4(0.4f, 0.4f, 0.4f, 1));

        // 現在の音量バー（緑）
        float filledW = barMaxW * g_Volume;              // 音量に応じた幅
        Sprite_Draw(g_WhiteTex, barX, barY, filledW, barH, XMFLOAT4(0.2f, 0.9f, 0.2f, 1));
    }                                                    // 終了

    //==========================================================================
    // "Volume" ラベル（バーの下、中央揃え）
    //==========================================================================
    if (g_TextVolumeLabelTex >= 0)                       // Volumeラベルテクスチャ有効
    {                                                    // 開始
        const float w = (float)Texture_Width(g_TextVolumeLabelTex);   // テクスチャ幅
        const float h = (float)Texture_Height(g_TextVolumeLabelTex);  // テクスチャ高さ
        const float barY = sh * 0.55f;                   // バーのY座標
        const float barH = 32.0f;                        // バーの高さ
        const float labelGap = 15.0f;                    // バーとラベルの間隔

        const float x = (sw - w) * 0.5f;                 // 中央揃え
        const float y = barY + barH + labelGap;          // バーの下

        Sprite_Draw(g_TextVolumeLabelTex, x, y, w, h, XMFLOAT4(1, 1, 1, 1)); // 描画
    }                                                    // 終了
}
//==============================================================================
// 終了判定
//==============================================================================
bool Option_IsEnd()                                      // 終了判定
{                                                        // 開始
    if (g_End)                                           // 終了フラグON
    {                                                    // 開始
        g_End = false;                                   // ワンショット消費
        return true;                                     // 終了
    }                                                    // 終了
    return false;                                        // 継続
}                                                        // 終了