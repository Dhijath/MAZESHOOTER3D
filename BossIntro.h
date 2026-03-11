/*==============================================================================

   ボス登場演出 [BossIntro.h]
                                                         Author : 51106
                                                         Date   : 2026/03/13
--------------------------------------------------------------------------------
   ・ボス部屋入場時にカメラがボスを舐め回す演出
   ・演出中はプレイヤー入力を無効化 (Player_SetEnable = false)
   ・フェーズ: ORBIT → RETURN → DONE

==============================================================================*/
#pragma once
#ifndef BOSS_INTRO_H
#define BOSS_INTRO_H

#include <DirectXMath.h>

// 演出を開始する（ボスのスポーン座標を渡す）
void BossIntro_Start(const DirectX::XMFLOAT3& bossSpawnPos);

// 毎フレーム更新（演出中のみカメラ上書き）
void BossIntro_Update(double dt);

// 演出中は true
bool BossIntro_IsPlaying();

#endif // BOSS_INTRO_H
