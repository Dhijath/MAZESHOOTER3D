/*==============================================================================

   HUD [HUD.h]
                                                         Author : 51106
                                                         Date   : 2026/02/18
--------------------------------------------------------------------------------
==============================================================================*/

#include "Item.h"
#ifndef HUD_H
#define HUD_H

void HUD_Initialize();
void HUD_Finalize();
void HUD_Draw();
// 取得済みアイテムを記録する（Pickup時に呼ぶ）
// type : 取得したアイテム種類
void HUD_AddCollectedItem(ItemType type);

void HUD_Update(double elapsed_time);   // モード表示タイマー更新
void HUD_NotifyModeChange(bool isBeam); // 武器モード切り替え通知

#endif // HUD_H