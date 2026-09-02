/*==============================================================================
   ショップ [Shop.cpp]
   Author : 51106
   Date   : 2026/06/12
==============================================================================*/
#include "Shop.h"
#include "WaveManager.h"
#include "WeaponDef.h"
#include "AssemblyScreen.h"
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
    static XMFLOAT3 g_Pos       = {};
    static bool     g_IsOpen    = false;
    static int      g_ShopBudget = 0;   // ショップを開いた時点の所持金（購入額算出用）
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
    // ショップを開いたまま終了する場合はアセンブリ画面も後始末する
    if (g_IsOpen) { AssemblyScreen_SetShopMode(false, 0); AssemblyScreen_Finalize(); }

    if (g_pDWTitle) { g_pDWTitle->Release(); delete g_pDWTitle; g_pDWTitle = nullptr; }
    if (g_pDWItem)  { g_pDWItem->Release();  delete g_pDWItem;  g_pDWItem  = nullptr; }
    if (g_pDWInfo)  { g_pDWInfo->Release();  delete g_pDWInfo;  g_pDWInfo  = nullptr; }
    UnloadAudio(g_SeOpen);   g_SeOpen   = -1;
    UnloadAudio(g_SeCursor); g_SeCursor = -1;
    UnloadAudio(g_SeSelect); g_SeSelect = -1;
    UnloadAudio(g_SeError);  g_SeError  = -1;
    g_IsOpen = false;
}

void Shop_Update(double elapsed_time)
{
    if (!g_IsOpen)
    {
        // 近くでEキー/Aボタン→アセンブリ画面をショップとして開く
        const bool inRange = (DistToPlayer() <= INTERACT_DIST);
        if (inRange && (KeyLogger_IsTrigger(KK_E) || PadLogger_IsTrigger(PAD_A)))
        {
            // 装備中の武器を初期値、所持金を予算にしてショップモードで開く
            AssemblyScreen_SetDefaults(
                static_cast<WeaponID>(Player_GetRightWeaponIndex()),
                static_cast<WeaponID>(Player_GetLeftWeaponIndex()));
            AssemblyScreen_Initialize();                       // ※ SetDefaults の後
            g_ShopBudget = WaveManager_GetCredits();
            AssemblyScreen_SetShopMode(true, g_ShopBudget);    // ※ Initialize の後

            g_IsOpen = true;
            PlayAudio(g_SeOpen, false);
        }
        return;
    }

    // ショップ表示中はアセンブリ画面（ショップモード）を駆動する。
    // 開いている間はゲームが凍結されるためプレイヤーは移動できない（＝距離で閉じない）。
    if (AssemblyScreen_Update(elapsed_time))
    {
        if (!AssemblyScreen_WasCancelled())
        {
            // 購入確定：装備中から変更したぶんの差額を消費し、両腕を装備する
            const int spent = g_ShopBudget - AssemblyScreen_GetRemainingCredits();
            if (spent > 0) WaveManager_SpendCredits(spent);

            Player_SetNormalWeaponIndex(static_cast<int>(AssemblyScreen_GetRightWeapon()));
            Player_SetLeftWeaponIndex  (static_cast<int>(AssemblyScreen_GetLeftWeapon()));
            PlayAudio(g_SeSelect, false);
        }

        AssemblyScreen_Finalize();
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

    // ショップ本体はアセンブリ画面（ショップモード）をそのまま描画する
    AssemblyScreen_Draw();

    // 操作ヒント（アセンブリ準拠。ショップなので Back → Close）
    InputHint_Draw(
        "{W}{S} Move    {ENTER} Set / Buy    {TAB} Jump    {ESC} Close",
        "{DPAD_UP}{DPAD_DN} Move    {A} Set / Buy    {LB}{RB} Jump    {B} Close");
}

bool Shop_IsOpen() { return g_IsOpen; }
