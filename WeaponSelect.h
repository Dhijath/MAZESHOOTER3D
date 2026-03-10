/*==============================================================================

   武器選択画面 [WeaponSelect.h]
   Author : 51106
   Date   : 2026/03/09

--------------------------------------------------------------------------------
   タイトルからゲーム開始前に通常スロット武器を選択する画面
   Normal / Shotgun / Missile を左右キーで選び、Enter/Aで決定
==============================================================================*/

#pragma once

// 選択結果
enum class WeaponSelectResult
{
    None,       // 未決定
    Decided,    // 決定（Enter / Aボタン）
};

// 初期化（画面遷移時に呼ぶ）
void               WeaponSelect_Initialize();

// 終了処理（アプリ終了時に呼ぶ）
void               WeaponSelect_Finalize();

// 更新（経過時間[s]）
void               WeaponSelect_Update(double elapsed_time);

// 描画
void               WeaponSelect_Draw();

// 現在の選択結果を返す
WeaponSelectResult WeaponSelect_GetResult();

// 選択された武器インデックスを返す（0=Normal 1=Shotgun 2=Missile）
int                WeaponSelect_GetSelectedIndex();
