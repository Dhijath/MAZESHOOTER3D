/*==============================================================================
   ショップ [Shop.h]
   Author : 51106
   Date   : 2026/06/12
--------------------------------------------------------------------------------
   サバイバルモードの武器購入ショップ（フェーズ問わずいつでも利用可）。
   スポーン地点に配置し、近づいてEキー/Aボタンで開く。離れると自動で閉じる。
==============================================================================*/
#pragma once
#include <DirectXMath.h>

void Shop_Initialize(const DirectX::XMFLOAT3& pos);
void Shop_Finalize();
void Shop_Update(double elapsed_time);
void Shop_Draw();       // 3Dオブジェクト（ビルボード）
void Shop_DrawUI();     // 購入UIオーバーレイ

bool Shop_IsOpen();
