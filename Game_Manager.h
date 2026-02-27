/*==============================================================================

   ゲーム管理 [Game_Manager.h]
   Author : 51106
   Date   : 2026/02/08

--------------------------------------------------------------------------------
   ゲーム全体の状態管理（タイトル／プレイ／オプション／リザルト／クリア／終了）
==============================================================================*/

#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

// ゲーム状態の列挙
enum class GameState
{
    Title,      // タイトル画面
    Playing,    // ゲーム中
    Option,     // オプション（音量等）
    Result,     // ゲームオーバー
    Clear,      // ゲームクリア
    Exit        // アプリ終了
};

// 初期化
void GameManager_Initialize();

// 終了
void GameManager_Finalize();

// 更新（経過時間[s]）
void GameManager_Update(double elapsed_time);

// 描画
void GameManager_Draw();

#endif // GAMEMANAGER_H