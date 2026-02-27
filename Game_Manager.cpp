/*==============================================================================

   ゲーム管理 [Game_Manager.cpp]
   Author : 51106
   Date   : 2026/02/08

--------------------------------------------------------------------------------
   状態: タイトル / プレイ / オプション / リザルト / クリア / 終了
   目的:
     - フェードと状態遷移を一元管理
     - BGMは遷移時に差し替え（旧をUnload→新をLoad&Loop）
     - 　ゴールを3回踏むとクリア
==============================================================================*/

#include "Game_Manager.h"
#include "Title.h"
#include "Option.h"
#include "game.h"
#include "player.h"
#include "Score.h"
#include "fade.h"
#include "Audio.h"
#include "key_logger.h"
#include "Clear.h"
#include "Result.h"
#include "Enemy.h"
#include "map.h"
#include <cstdint>
#include "EnemyAPI.h"
#include "pad_logger.h"

// 現在/次の状態
static GameState g_GameState = GameState::Title;
static GameState g_NextState = GameState::Title;

// フェード中に多重遷移を防ぐフラグ（フェードアウト開始〜完了まで true）
static bool g_IsTransitioning = false;

// 現在鳴っているBGMのハンドル（Unload 用）
static int g_CurrentBgmId = -1;

// ダンジョン生成用シード
static std::uint32_t g_DungeonSeed = 12345u;

// ゴール判定のクールダウン（秒）
static double g_GoalCooldown = 0.0;
// ゴール到達によるダンジョン再生成待ちフラグ
static bool g_PendingDungeonRegenerate = false;

// BGMパス（必要に応じて差し替え）
static const char* BGM_TITLE = "resource/sound/maou_bgm_cyber39.wav";                              
static const char* BGM_GAME = "resource/sound/maou_bgm_cyber42.wav";                               
static const char* BGM_OPTION = "resource/sound/maou_bgm_cyber40.wav";                             
static const char* BGM_RESULT = "resource/sound/maou_bgm_cyber45_1.wav"; // リザルト/クリア兼用    

int g_PlayerWarpSE = -1;
int g_PlayerclearSE = -1;


//------------------------------------------------------------------------------
// 内部: BGMを差し替えてループ再生（前のBGMはUnloadして重なり回避）
//------------------------------------------------------------------------------
static void StartBgmLoop(const char* path)
{
    const float BGM_VOLUME = 0.35f; // ここで全BGMの基準音量を決める（0.0f～1.0f）

    if (g_CurrentBgmId >= 0)
    {
        UnloadAudio(g_CurrentBgmId);
        g_CurrentBgmId = -1;
    }

    if (path && path[0] != '\0')
    {
        g_CurrentBgmId = LoadAudioWithVolume(path, BGM_VOLUME);
        if (g_CurrentBgmId >= 0)
        {
            PlayAudio(g_CurrentBgmId, /*Loop=*/true);
        }
    }
}


//------------------------------------------------------------------------------
// 内部: 遷移開始（フェードアウト + 次状態予約 + 以降の判定を停止）
//------------------------------------------------------------------------------
static void BeginTransition(GameState next, const char* nextBgmPath)
{
    if (g_IsTransitioning) return;                 // 多重開始を防止
    g_IsTransitioning = true;
    g_NextState = next;
    Fade_Start(1.0, /*out=*/true, { 1,1,1 });     // 白フェードアウト開始
    StartBgmLoop(nextBgmPath);                     // 次シーンBGMへ差し替え
}

//------------------------------------------------------------------------------
// 初期化
//------------------------------------------------------------------------------
void GameManager_Initialize()
{
    Title_Initialize();               // まずはタイトル
    StartBgmLoop(BGM_TITLE);          // タイトルBGM開始
    g_IsTransitioning = false;        // 遷移中フラグOFF
    g_GoalCooldown = 0.0;             // クールダウンリセット
    Fade_Start(1.0, /*isFadeOut=*/false, { 1,1,1 }); // 起動フェードイン
    g_PlayerWarpSE = LoadAudioWithVolume("resource/sound/warp.wav", 0.5f);
    g_PlayerclearSE = LoadAudioWithVolume("resource/sound/clear.wav", 0.5f);
}

//------------------------------------------------------------------------------
// 終了
//------------------------------------------------------------------------------
void GameManager_Finalize()
{
    Title_Finalize();
    Option_Finalize();
    Game_Finalize();
    Clear_Finalize();
    Result_Finalize();
    UnloadAudio(g_PlayerWarpSE);

    if (g_CurrentBgmId >= 0)
    {
        UnloadAudio(g_CurrentBgmId);
        g_CurrentBgmId = -1;
    }

    Enemy_UnloadSE();
}

//------------------------------------------------------------------------------
// 更新
//------------------------------------------------------------------------------
//==============================================================================
// 更新
//
// ■役割
// ・現在の状態に応じた更新処理を呼び出す
// ・フェードアウト完了後に状態遷移・初期化を実行する
// ・ゴール到達によるダンジョン再生成もフェード完了後に処理する
//
// ■引数
// ・elapsed_time : 経過時間（秒）
//==============================================================================
void GameManager_Update(double elapsed_time)
{
    switch (g_GameState)
    {
    case GameState::Title:
    {
        Title_Update(elapsed_time);

        if (!g_IsTransitioning)
        {
            TitleResult tr = Title_GetResult();
            if (tr == TitleResult::Start)
            {
                BeginTransition(GameState::Playing, BGM_GAME);
            }
            else if (tr == TitleResult::Option)
            {
                BeginTransition(GameState::Option, BGM_OPTION);
            }
            else if (tr == TitleResult::Exit)
            {
                g_GameState = GameState::Exit;
            }

            if (Title_IsEnd())
            {
                BeginTransition(GameState::Playing, BGM_GAME);
            }
        }
        break;
    }

    case GameState::Playing:
    {
        if (g_IsTransitioning) break;

        Game_Update(elapsed_time);

        if (g_GoalCooldown > 0.0)
        {
            g_GoalCooldown -= elapsed_time;
        }

        // ゲームオーバー：フェードアウト後にリザルトへ遷移
        if (!Player_IsEnable())
        {
            BeginTransition(GameState::Result, BGM_RESULT);
            break;
        }

        // ゴール到達判定（クールダウン中はスキップ）
        if (g_GoalCooldown <= 0.0 && Map_IsPlayerReachedGoal())
        {
            Map_AddGoalReachCount();

            if (Map_IsClearConditionMet())
            {
                // 3回到達：クリア画面へフェード遷移
                PlayAudio(g_PlayerclearSE);
                BeginTransition(GameState::Clear, BGM_RESULT);
            }
            else
            {
                // 3回未満：フェードアウト完了後にダンジョン再生成を予約
                g_PendingDungeonRegenerate = true;
                g_IsTransitioning = true;
                Fade_Start(0.5, true, { 0.0f, 0.0f, 0.0f });
                PlayAudio(g_PlayerWarpSE);
            }
        }

        break;
    }

    case GameState::Option:
    {
        if (!g_IsTransitioning)
        {
            Option_Update(elapsed_time);

            if (Option_IsEnd())
            {
                BeginTransition(GameState::Title, BGM_TITLE);
            }
        }
        break;
    }

    case GameState::Result:
    {
        // Enter または Aボタン でタイトルへ
        if (!g_IsTransitioning &&
            (KeyLogger_IsTrigger(KK_ENTER) || PadLogger_IsTrigger(PAD_A)))
        {
            BeginTransition(GameState::Title, BGM_TITLE);
        }
        break;
    }

    case GameState::Clear:
    {
        // Enter または Aボタン でタイトルへ
        if (!g_IsTransitioning &&
            (KeyLogger_IsTrigger(KK_ENTER) || PadLogger_IsTrigger(PAD_A)))
        {
            BeginTransition(GameState::Title, BGM_TITLE);
        }
        break;
    }

    case GameState::Exit:
        PostQuitMessage(0);
        break;
    }

    //--------------------------------------------------------------------------
    // フェードアウト完了で遷移を確定
    //--------------------------------------------------------------------------
    if (g_IsTransitioning && Fade_IsOutEnd())
    {
        // ゴール到達によるダンジョン再生成（状態は Playing のまま）
        if (g_PendingDungeonRegenerate)
        {
            g_PendingDungeonRegenerate = false;

            Map_GenerateDungeon(++g_DungeonSeed);
            Score_Addscore(5000);
            Map_RegisterFloors();
            Player_SetPosition(Map_GetSpawnPosition(), true);
            Enemy_Finalize();
            Enemy_Initialize({});

            const auto& spawns = Map_GetEnemySpawnPositions();
            for (const auto& p : spawns)
            {
                Enemy_Spawn(p);
            }

            g_GoalCooldown = 1.0;

            // 再生成完了後にフェードイン
            Fade_StartIn(0.5, { 0.0f, 0.0f, 0.0f });
            g_IsTransitioning = false;
            return;
        }

        // 通常の状態遷移
        g_GameState = g_NextState;

        if (g_GameState == GameState::Playing)
        {
            Game_Initialize();
        }
        else if (g_GameState == GameState::Title)
        {
            Title_Initialize();
            Map_ResetGoalReachCount();
        }
        else if (g_GameState == GameState::Option)
        {
            Option_Initialize();
        }
        else if (g_GameState == GameState::Result)
        {
            Result_Initialize();
        }
        else if (g_GameState == GameState::Clear)
        {
            Clear_Initialize();
        }

        Fade_StartIn(1.0, { 1.0f, 1.0f, 1.0f });
        g_IsTransitioning = false;
    }
}
//------------------------------------------------------------------------------
// 描画
//------------------------------------------------------------------------------
void GameManager_Draw()
{
    switch (g_GameState)
    {
    case GameState::Title:   Title_Draw();   break;
    case GameState::Playing: Game_Draw();    break;
    case GameState::Option:  Option_Draw();  break;
    case GameState::Result:  Result_Draw();  break;
    case GameState::Clear:   Clear_Draw();   break;
    case GameState::Exit:    /* 何も描かない */ break;
    }

    // フェードは最後に重ねる
    Fade_Draw();
}