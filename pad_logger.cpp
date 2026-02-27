/*==============================================================================

   ゲームパッド入力管理 [pad_logger.cpp]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
   XInputゲームパッドの入力状態を管理し、押下・トリガー・リリースの判定を提供する
   - IsPressed : ボタンが押されている間ずっと true
   - IsTrigger : ボタンが押された瞬間のみ true
   - IsRelease : ボタンが離された瞬間のみ true
==============================================================================*/

#include "pad_logger.h"
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

// -----------------------------------------------------------------------------
// グローバル変数
// -----------------------------------------------------------------------------
static XINPUT_STATE g_PadNow{};       // 現在のパッド状態
static XINPUT_STATE g_PadPrev{};      // 前フレームのパッド状態
static WORD g_TriggerState = 0;       // トリガー判定用（押した瞬間）
static WORD g_ReleaseState = 0;       // リリース判定用（離した瞬間）
static bool g_PadConnected = false;   // 接続状態

//==============================================================================
// 初期化
//==============================================================================
void PadLogger_Initialize()
{
    ZeroMemory(&g_PadNow, sizeof(g_PadNow));
    ZeroMemory(&g_PadPrev, sizeof(g_PadPrev));
    g_TriggerState = 0;
    g_ReleaseState = 0;
    g_PadConnected = false;
}

//==============================================================================
// 更新処理：前フレームと現フレームの差分からトリガー・リリースを判定
//==============================================================================
void PadLogger_Update()
{
    // 前フレームの状態を保存
    g_PadPrev = g_PadNow;

    // 現在の状態を取得
    ZeroMemory(&g_PadNow, sizeof(g_PadNow));
    const DWORD result = XInputGetState(0, &g_PadNow);
    g_PadConnected = (result == ERROR_SUCCESS);

    // 接続されていない場合はリセット
    if (!g_PadConnected)
    {
        ZeroMemory(&g_PadNow, sizeof(g_PadNow));
        ZeroMemory(&g_PadPrev, sizeof(g_PadPrev));
        g_TriggerState = 0;
        g_ReleaseState = 0;
        return;
    }

    // トリガー判定：前フレーム OFF かつ 今フレーム ON
    const WORD now = g_PadNow.Gamepad.wButtons;
    const WORD prev = g_PadPrev.Gamepad.wButtons;

    g_TriggerState = (prev ^ now) & now;

    // リリース判定：前フレーム ON かつ 今フレーム OFF
    g_ReleaseState = (prev ^ now) & prev;
}

//==============================================================================
// 押下判定：ボタンが押されている間ずっと true
//==============================================================================
bool PadLogger_IsPressed(WORD button)
{
    if (!g_PadConnected) return false;
    return (g_PadNow.Gamepad.wButtons & button) != 0;
}

//==============================================================================
// トリガー判定：ボタンが押された瞬間のみ true
//==============================================================================
bool PadLogger_IsTrigger(WORD button)
{
    if (!g_PadConnected) return false;
    return (g_TriggerState & button) != 0;
}

//==============================================================================
// リリース判定：ボタンが離された瞬間のみ true
//==============================================================================
bool PadLogger_IsRelease(WORD button)
{
    if (!g_PadConnected) return false;
    return (g_ReleaseState & button) != 0;
}

//==============================================================================
// デッドゾーン適用
//==============================================================================
static float ApplyDeadZone(SHORT value, SHORT deadZone)
{
    if (value > deadZone)
    {
        return (value - deadZone) / float(32767 - deadZone);
    }
    if (value < -deadZone)
    {
        return (value + deadZone) / float(32768 - deadZone);
    }
    return 0.0f;
}

//==============================================================================
// 左スティック取得（-1.0 ~ 1.0、デッドゾーン適用済み）
//==============================================================================
void PadLogger_GetLeftStick(float* outX, float* outY)
{
    if (!g_PadConnected)
    {
        *outX = 0.0f;
        *outY = 0.0f;
        return;
    }

    constexpr SHORT DEAD_ZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

    *outX = ApplyDeadZone(g_PadNow.Gamepad.sThumbLX, DEAD_ZONE);
    *outY = ApplyDeadZone(g_PadNow.Gamepad.sThumbLY, DEAD_ZONE);
}

//==============================================================================
// 右スティック取得（-1.0 ~ 1.0、デッドゾーン適用済み）
//==============================================================================
void PadLogger_GetRightStick(float* outX, float* outY)
{
    if (!g_PadConnected)
    {
        *outX = 0.0f;
        *outY = 0.0f;
        return;
    }

    constexpr SHORT DEAD_ZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

    *outX = ApplyDeadZone(g_PadNow.Gamepad.sThumbRX, DEAD_ZONE);
    *outY = ApplyDeadZone(g_PadNow.Gamepad.sThumbRY, DEAD_ZONE);
}

//==============================================================================
// 左トリガー取得（0.0 ~ 1.0）
//==============================================================================
float PadLogger_GetLeftTrigger()
{
    if (!g_PadConnected) return 0.0f;

    const BYTE trigger = g_PadNow.Gamepad.bLeftTrigger;
    constexpr BYTE DEAD_ZONE = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    if (trigger < DEAD_ZONE) return 0.0f;

    return (trigger - DEAD_ZONE) / float(255 - DEAD_ZONE);
}

//==============================================================================
// 右トリガー取得（0.0 ~ 1.0）
//==============================================================================
float PadLogger_GetRightTrigger()
{
    if (!g_PadConnected) return 0.0f;

    const BYTE trigger = g_PadNow.Gamepad.bRightTrigger;
    constexpr BYTE DEAD_ZONE = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    if (trigger < DEAD_ZONE) return 0.0f;

    return (trigger - DEAD_ZONE) / float(255 - DEAD_ZONE);
}

//==============================================================================
// 接続状態取得
//==============================================================================
bool PadLogger_IsConnected()
{
    return g_PadConnected;
}