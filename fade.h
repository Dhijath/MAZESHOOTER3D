/*==============================================================================

   フェード [fade.h]
                                                         Author : 51106
                                                         Date   : 2026/02/15
--------------------------------------------------------------------------------
   フェード機能のヘッダ。基本APIに加えて、使いやすいラッパーを宣言してあります。
==============================================================================*/
#pragma once

#include <DirectXMath.h>

// フェード状態列挙
enum FadeState
{
    FADE_STATE_NONE = 0,
    FADE_STATE_OUT,
    FADE_STATE_FINISHED_OUT,
    FADE_STATE_IN,
    FADE_STATE_FINISHED_IN
};

// 基本API（既存実装と同じもの）
void Fade_Initialize();
void Fade_Finalize();
void Fade_Update(double elapsed_time);
void Fade_Draw();

// time: フェードにかける秒数
// isFadeOut: true = フェードアウト（画面が暗くなる）, false = フェードイン（明るくなる）
// color: フェード色（RGB）
void Fade_Start(double time, bool isFadeOut, DirectX::XMFLOAT3 color);

// 現在のフェード状態を取得
FadeState Fade_GetState();

// 便利ラッパー（ゲーム側で使いやすい関数）
// 宣言だけここに置いておけば、他ファイルから直接使えます。

// 便利ラッパー
void Fade_StartOut(double time = 1.0, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f });

void Fade_StartIn(double time = 1.0, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3{ 0.0f,0.0f,0.0f });



bool Fade_IsOutEnd();   // フェードアウト（暗転）が完了したか

bool Fade_IsInEnd();    // フェードイン（明転）が完了したか

bool Fade_IsActive();   // フェードが進行中か（true = 進行中）