/*==============================================================================

   ゲームパッド入力管理 [pad_logger.h]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
   XInputゲームパッドの入力状態を管理し、押下・トリガー・リリースの判定を提供する
   - IsPressed : ボタンが押されている間ずっと true
   - IsTrigger : ボタンが押された瞬間のみ true
   - IsRelease : ボタンが離された瞬間のみ true
   - 左右スティックの入力取得（デッドゾーン適用済み）
   - 左右トリガーの入力取得
==============================================================================*/
#ifndef PAD_LOGGER_H
#define PAD_LOGGER_H

#include <Windows.h>

//==============================================================================
// ゲームパッドボタン定義
//==============================================================================
enum PadLogger_Buttons
{
    PAD_DPAD_UP = 0x0001,  // 十字キー上
    PAD_DPAD_DOWN = 0x0002,  // 十字キー下
    PAD_DPAD_LEFT = 0x0004,  // 十字キー左
    PAD_DPAD_RIGHT = 0x0008,  // 十字キー右
    PAD_START = 0x0010,  // STARTボタン
    PAD_BACK = 0x0020,  // BACKボタン（SELECTボタン）
    PAD_LEFT_THUMB = 0x0040,  // 左スティック押し込み（L3）
    PAD_RIGHT_THUMB = 0x0080,  // 右スティック押し込み（R3）
    PAD_LEFT_SHOULDER = 0x0100,  // 左ショルダー（LBボタン）
    PAD_RIGHT_SHOULDER = 0x0200,  // 右ショルダー（RBボタン）
    PAD_A = 0x1000,  // Aボタン（Xbox：A、PS：×）
    PAD_B = 0x2000,  // Bボタン（Xbox：B、PS：○）
    PAD_X = 0x4000,  // Xボタン（Xbox：X、PS：□）
    PAD_Y = 0x8000,  // Yボタン（Xbox：Y、PS：△）
};

//==============================================================================
// 関数プロトタイプ
//==============================================================================
void PadLogger_Initialize();

void PadLogger_Update();

// ボタン判定
bool PadLogger_IsPressed(WORD button);
bool PadLogger_IsTrigger(WORD button);
bool PadLogger_IsRelease(WORD button);

// スティック取得（-1.0 ~ 1.0、デッドゾーン適用済み）
void PadLogger_GetLeftStick(float* outX, float* outY);
void PadLogger_GetRightStick(float* outX, float* outY);

// トリガー取得（0.0 ~ 1.0）
float PadLogger_GetLeftTrigger();
float PadLogger_GetRightTrigger();

// 接続状態
bool PadLogger_IsConnected();

#endif // !PAD_LOGGER_H