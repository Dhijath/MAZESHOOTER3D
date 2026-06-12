/*==============================================================================
   ショップ [Shop.cpp]
   Author : 51106
   Date   : 2026/06/12
==============================================================================*/
#include "Shop.h"
#include "WaveManager.h"
#include "WeaponDef.h"
#include "player.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "UIInput.h"
#include "billboard.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "DirectWrite.h"
#include "audio.h"
#include "input_hint.h"
#include <d2d1helper.h>
#include <DirectXMath.h>
#include <cstdio>
#include <cmath>
using namespace DirectX;

namespace
{
    static XMFLOAT3 g_Pos     = {};
    static bool     g_IsOpen  = false;
    static int      g_Cursor  = 0;
    static int      g_TexShop = -1;
    static int      g_WhiteTex = -1;
    static int      g_SeOpen   = -1;
    static int      g_SeCursor = -1;
    static int      g_SeSelect = -1;
    static int      g_SeError  = -1;

    static constexpr float INTERACT_DIST = 4.0f; // インタラクト可能距離

    // DirectWrite 生成時の基準フォントサイズ。
    // 実際の描画サイズは DrawTextFit が枠サイズ比から拡縮するため、
    // この値自体はレイアウトに影響しない（DrawAt 内の縦中心計算の基準）。
    static constexpr float BASE_FONT_SIZE = 32.0f;

    static DirectWrite* g_pDWTitle  = nullptr;
    static DirectWrite* g_pDWItem   = nullptr;
    static DirectWrite* g_pDWInfo   = nullptr;

    // 仮想座標 (cx, cy) を中心に、仮想単位 fontSize の高さで文字列を描く。
    // SetScale は座標とグリフを同率で拡縮するため、倍率 k を掛けた分だけ
    // 座標を 1/k して渡すことで「位置はそのまま・文字だけ枠比サイズ」になる。
    // cy はグリフの縦中心（DrawAt の top = cy - 0.75*fontSize を補正済み）。
    static void DrawTextFit(DirectWrite* dw, const char* text,
                            float cx, float cy, float halfW, float fontSize,
                            const D2D1_COLOR_F& color,
                            float scaleX, float scaleY)
    {
        const float k = fontSize / BASE_FONT_SIZE;
        const float cyTop = cy + fontSize * 0.25f; // DrawAt の縦アンカーを中心へ補正
        dw->SetScale(scaleX * k, scaleY * k);
        dw->BeginBatch();
        dw->DrawAt(text, cx / k, cyTop / k, halfW / k, color);
        dw->EndBatch();
        dw->SetScale(1.0f, 1.0f);
    }

    static float DistToPlayer()
    {
        const XMFLOAT3 p = Player_GetPosition();
        const float dx = p.x - g_Pos.x;
        const float dz = p.z - g_Pos.z;
        return sqrtf(dx*dx + dz*dz);
    }
}

void Shop_Initialize(const XMFLOAT3& pos)
{
    g_Pos    = pos;
    g_IsOpen = false;
    g_Cursor = 0;

    g_TexShop  = Texture_Load(L"resource/texture/white.png"); // TODO: 専用テクスチャ
    g_WhiteTex = Texture_Load(L"resource/texture/white.png");

    if (g_SeOpen   < 0) g_SeOpen   = LoadAudio("resource/Sound/ui_select.wav");
    if (g_SeCursor < 0) g_SeCursor = LoadAudio("resource/Sound/ui_cursor_move.wav");
    if (g_SeSelect < 0) g_SeSelect = LoadAudio("resource/Sound/ui_select.wav");
    if (g_SeError  < 0) g_SeError  = LoadAudio("resource/Sound/ui_cancel.wav");

    if (!g_pDWTitle)
    {
        static FontData fd;
        fd.font = Font::Arial; fd.fontWeight = DWRITE_FONT_WEIGHT_BOLD;
        fd.fontStyle = DWRITE_FONT_STYLE_NORMAL; fd.fontStretch = DWRITE_FONT_STRETCH_NORMAL;
        fd.fontSize = BASE_FONT_SIZE; fd.localeName = L"en-us";
        fd.textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
        fd.Color = D2D1::ColorF(1,1,1,1);
        g_pDWTitle = new DirectWrite(&fd); g_pDWTitle->Init();
        g_pDWTitle->SetWordWrapping(false);
    }
    if (!g_pDWItem)
    {
        static FontData fd;
        fd.font = Font::Arial; fd.fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
        fd.fontStyle = DWRITE_FONT_STYLE_NORMAL; fd.fontStretch = DWRITE_FONT_STRETCH_NORMAL;
        fd.fontSize = BASE_FONT_SIZE; fd.localeName = L"en-us";
        fd.textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        fd.Color = D2D1::ColorF(1,1,1,1);
        g_pDWItem = new DirectWrite(&fd); g_pDWItem->Init();
        g_pDWItem->SetWordWrapping(false);
    }
    if (!g_pDWInfo)
    {
        static FontData fd;
        fd.font = Font::Arial; fd.fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
        fd.fontStyle = DWRITE_FONT_STYLE_NORMAL; fd.fontStretch = DWRITE_FONT_STRETCH_NORMAL;
        fd.fontSize = BASE_FONT_SIZE; fd.localeName = L"en-us";
        fd.textAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
        fd.Color = D2D1::ColorF(0.8f,0.8f,0.8f,1);
        g_pDWInfo = new DirectWrite(&fd); g_pDWInfo->Init();
        g_pDWInfo->SetWordWrapping(false);
    }
}

void Shop_Finalize()
{
    if (g_pDWTitle) { g_pDWTitle->Release(); delete g_pDWTitle; g_pDWTitle = nullptr; }
    if (g_pDWItem)  { g_pDWItem->Release();  delete g_pDWItem;  g_pDWItem  = nullptr; }
    if (g_pDWInfo)  { g_pDWInfo->Release();  delete g_pDWInfo;  g_pDWInfo  = nullptr; }
    UnloadAudio(g_SeOpen);   g_SeOpen   = -1;
    UnloadAudio(g_SeCursor); g_SeCursor = -1;
    UnloadAudio(g_SeSelect); g_SeSelect = -1;
    UnloadAudio(g_SeError);  g_SeError  = -1;
    g_IsOpen = false;
}

void Shop_Update(double)
{
    const bool inRange = (DistToPlayer() <= INTERACT_DIST);

    if (!g_IsOpen)
    {
        // 近くでEキー/Aボタン→開く（フェーズ問わずいつでも可）
        if (inRange && (KeyLogger_IsTrigger(KK_E) || PadLogger_IsTrigger(PAD_A)))
        {
            g_IsOpen = true;
            g_Cursor = 0;
            PlayAudio(g_SeOpen, false);
        }
        return;
    }

    // ショップから離れたら自動で閉じる
    if (!inRange)
    {
        g_IsOpen = false;
        return;
    }

    // ショップ操作
    if (UI_IsMoveUp())
    {
        g_Cursor = (g_Cursor + WEAPON_COUNT - 1) % WEAPON_COUNT;
        PlayAudio(g_SeCursor, false);
    }
    if (UI_IsMoveDown())
    {
        g_Cursor = (g_Cursor + 1) % WEAPON_COUNT;
        PlayAudio(g_SeCursor, false);
    }

    if (UI_IsConfirm())
    {
        const int cost = k_WeaponDefs[g_Cursor].cost;
        if (WaveManager_SpendCredits(cost))
        {
            // 右腕に装備（後で左右選択も追加できる）
            Player_SetNormalWeaponIndex(g_Cursor);
            PlayAudio(g_SeSelect, false);
        }
        else
        {
            PlayAudio(g_SeError, false); // クレジット不足
        }
    }

    if (UI_IsCancel())
    {
        g_IsOpen = false;
    }
}

void Shop_Draw()
{
    // ショップのビルボード表示（スポーン地点に目印）
    if (g_TexShop >= 0)
    {
        const XMFLOAT3 drawPos = { g_Pos.x, g_Pos.y + 2.0f, g_Pos.z };
        Billboard_Draw(g_TexShop, drawPos,
                       XMFLOAT2{ 2.0f, 2.0f },
                       XMFLOAT4{ 0.3f, 0.8f, 1.0f, 0.9f },
                       XMFLOAT4{ 0, 0,
                           (float)Texture_Width(g_TexShop),
                           (float)Texture_Height(g_TexShop) });
    }
}

void Shop_DrawUI()
{
    if (!g_IsOpen) return;
    if (!g_pDWTitle || !g_pDWItem || !g_pDWInfo) return;

    const float sw = (float)SPRITE_SCREEN_W;
    const float sh = (float)SPRITE_SCREEN_H;
    const float scaleX = (float)Direct3D_GetBackBufferWidth()  / 1600.0f;
    const float scaleY = (float)Direct3D_GetBackBufferHeight() / 900.0f;

    Direct3D_SetDepthEnable(false);
    Direct3D_SetBlendState(true);
    Sprite_Begin();

    // パネル（画面比率で管理）
    const float PNL_W   = sw * 0.62f;
    const float PNL_H   = sh * 0.80f;
    const float px      = sw * 0.5f - PNL_W * 0.5f;
    const float py      = sh * 0.5f - PNL_H * 0.5f;
    const float cx      = sw * 0.5f;
    const float ROW_H   = PNL_H / (WEAPON_COUNT + 1.5f);
    const float TITLE_H = ROW_H * 0.9f;
    const float LIST_Y  = py + TITLE_H + ROW_H * 0.3f;

    // 文字サイズは行・タイトルの高さ比で決定（枠が変われば文字も追従する）
    const float FONT_TITLE = TITLE_H * 0.60f;
    const float FONT_ITEM  = ROW_H   * 0.50f;
    const float FONT_INFO  = ROW_H   * 0.40f;

    // テキスト矩形（左端: 武器名 / 右端: 価格。パネル幅比で配置）
    // 矩形幅が文字列より狭いと改行されるため、想定最大文字数分の幅を確保する
    // （SetWordWrapping(false) で改行自体も無効化済み）
    const float LABEL_HALF_W = PNL_W * 0.28f;
    const float LABEL_CX     = px + PNL_W * 0.06f + LABEL_HALF_W; // 左揃え: 左端 6%
    const float PRICE_HALF_W = PNL_W * 0.18f;                     // "CR 100000" 9文字分
    const float PRICE_CX     = px + PNL_W * 0.94f - PRICE_HALF_W; // 右揃え: 右端 6%

    if (g_WhiteTex >= 0)
    {
        Sprite_Draw(g_WhiteTex, px, py, PNL_W, PNL_H, XMFLOAT4(0,0,0,0.8f));
        Sprite_Draw(g_WhiteTex, px,           py,          PNL_W, 2, XMFLOAT4(1,1,1,0.5f));
        Sprite_Draw(g_WhiteTex, px,           py+PNL_H-2,  PNL_W, 2, XMFLOAT4(1,1,1,0.5f));
        Sprite_Draw(g_WhiteTex, px,           py,          2, PNL_H, XMFLOAT4(1,1,1,0.5f));
        Sprite_Draw(g_WhiteTex, px+PNL_W-2,  py,          2, PNL_H, XMFLOAT4(1,1,1,0.5f));
        // タイトル下区切り線
        Sprite_Draw(g_WhiteTex, px + PNL_W*0.05f, py + TITLE_H,
                    PNL_W * 0.9f, 1, XMFLOAT4(1,1,1,0.3f));
    }

    // タイトル（所持金表示）：タイトル帯の縦中心に配置
    char creditBuf[32];
    snprintf(creditBuf, sizeof(creditBuf), "SHOP   CR: %d", WaveManager_GetCredits());
    DrawTextFit(g_pDWTitle, creditBuf,
                cx, py + TITLE_H * 0.5f, PNL_W * 0.45f, FONT_TITLE,
                D2D1::ColorF(0.4f, 0.9f, 1.0f, 1.0f), scaleX, scaleY);

    // 武器リスト：各行の縦中心に配置
    for (int i = 0; i < WEAPON_COUNT; ++i)
    {
        const float ry  = LIST_Y + i * ROW_H;
        const bool  sel = (i == g_Cursor);

        if (sel && g_WhiteTex >= 0)
            Sprite_Draw(g_WhiteTex, px + 2, ry, PNL_W - 4, ROW_H - 2, XMFLOAT4(1,1,1,0.12f));

        const D2D1_COLOR_F nameCol = sel
            ? D2D1::ColorF(1.0f, 0.9f, 0.3f, 1.0f)
            : D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f);

        char priceBuf[32];
        snprintf(priceBuf, sizeof(priceBuf), "CR %d", k_WeaponDefs[i].cost);

        const float rowCY = ry + ROW_H * 0.5f;

        DrawTextFit(g_pDWItem, k_WeaponDefs[i].name,
                    LABEL_CX, rowCY, LABEL_HALF_W, FONT_ITEM,
                    nameCol, scaleX, scaleY);

        DrawTextFit(g_pDWInfo, priceBuf,
                    PRICE_CX, rowCY, PRICE_HALF_W, FONT_INFO,
                    D2D1::ColorF(0.7f, 0.9f, 0.5f, 1.0f), scaleX, scaleY);
    }

    Direct3D_SetDepthEnable(true);

    // ヒント
    Direct3D_BindMainRenderTarget();
    InputHint_Draw(
        "{UP}{DOWN} Move    {ENTER} Buy    {ESC} Close",
        "{DPAD_UP}{DPAD_DN} Move    {A} Buy    {B} Close");
}

bool Shop_IsOpen() { return g_IsOpen; }
