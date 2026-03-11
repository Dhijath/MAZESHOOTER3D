/*==============================================================================

   ダメージポップアップ [DamagePopup.h]
                                                         Author : 51106
                                                         Date   : 2026/03/13
--------------------------------------------------------------------------------
   ・敵へのダメージ数字をワールド座標から浮き上がらせる
   ・suji.png の UV カットで数字を 1 桁ずつ描画
   ・生存時間 1.0秒、上昇速度 1.5m/s、alpha = life^2

==============================================================================*/
#pragma once
#ifndef DAMAGE_POPUP_H
#define DAMAGE_POPUP_H

#include <DirectXMath.h>

void DamagePopup_Initialize();
void DamagePopup_Finalize();

// ポップアップを登録（ダメージを受けた敵の位置 + ダメージ量）
void DamagePopup_Add(const DirectX::XMFLOAT3& worldPos, int damage);

void DamagePopup_Update(float dt);
void DamagePopup_Draw();

#endif // DAMAGE_POPUP_H
