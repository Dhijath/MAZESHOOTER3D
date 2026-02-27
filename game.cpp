/*==============================================================================

   ゲーム制御 [game.cpp]
                                                         Author : 51106
                                                         Date   : 2025/01/16
--------------------------------------------------------------------------------

   ・ゲーム全体の初期化 / 更新 / 描画 / 終了
   ・プレイヤー / カメラ / マップ / 弾 / エフェクト / エネミーを統括
   ・ダンジョン再生成（Rキー）を追加
   ・　エネミーとの衝突でダメージ処理

==============================================================================*/

#include "game.h"
#include "shader.h"
#include "Sampler.h"
#include "Meshfield.h"
#include "Light.h"
#include <DirectXMath.h>
using namespace DirectX;

#include "model.h"
#include "Player.h"
#include "Player_Camera.h"
#include "map.h"
#include "billboard.h"
#include "texture.h"
#include "sprite_anim.h"
#include "bullet.h"
#include "bullet_hit_effect.h"
#include "direct3d.h"
#include "cube.h"
#include "sprite.h"
#include "blob_shadow.h"
#include "FloorRegistry.h"
#include "key_logger.h"
#include "collision.h"

#include <cfloat>
#include <cstdint>
#include "shader3d.h"
#include "EnemyAPI.h"
#include "WallShader.h"
#include "Minimap.h"
#include "HUD.h"
#include "ItemManager.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "EnemyManager.h"




//==============================================================================
// 足元の「支持面Y」を探す（XZが重なっていて、上面が足元以下の中で最大のY）
// Map 構造に依存するため game.cpp 内部関数として定義
//==============================================================================
static bool GetSupportY_FromMapAABBs(
    const AABB& actor,
    float* outY)
{
    const float eps = 0.02f;

    float bestY = -FLT_MAX;
    bool found = false;

    for (int i = 0; i < Map_GetObjectsCount(); ++i)
    {
        const MapObject* objPtr = Map_GetObject(i);
        if (objPtr->KindId != 1) continue; // KIND_FLOOR だけ
        const AABB& obj = objPtr->Aabb;


        // XZ が重なっているか（簡易）
        const bool overlapX =
            actor.min.x <= obj.max.x && actor.max.x >= obj.min.x;
        const bool overlapZ =
            actor.min.z <= obj.max.z && actor.max.z >= obj.min.z;

        if (!overlapX || !overlapZ) continue;

        // 「上に乗れる面」条件
        const float topY = obj.max.y;
        if (topY <= actor.min.y + eps)
        {
            if (!found || topY > bestY)
            {
                bestY = topY;
                found = true;
            }
        }
    }

    if (found && outY) *outY = bestY;
    return found;
}

namespace
{
    // フィールド描画用のワールド行列
    XMMATRIX g_mtxWorld_Field;

    // テスト用テクスチャID
    int g_TexTest = -1;

    // ダンジョン生成用シード（Rで更新）

    std::uint32_t g_DungeonSeed = 12345u;
    static EnemyManager g_EnemyManager;
}

//==============================================================================
// ゲーム全体の初期化
//==============================================================================
void Game_Initialize()
{
    // 3D描画用ステートに戻す

    Shader3d_Begin();             // ← 3D用シェーダ・行列
    Direct3D_ConfigureOffScreenBuffer();

    EnemyBullet_Initialize();
    // 弾・被弾エフェクトの初期化
    Bullet_Initialize();
    BulletHitEffect_Initialize();

    // マップ初期化（ここでスポーン位置が確定する想定）
    Map_Initialize();


    // 念のため床登録を更新（支持面判定に使う場合がある）
    Map_RegisterFloors();

    // エネミー初期化（マネージャ初期化）
    g_EnemyManager.Initialize();
    const auto& spawns = Map_GetEnemySpawnPositions();
    for (int i = 0; i < static_cast<int>(spawns.size()); ++i)
    {
        // スポーン順番で種別を割り振る（3体に1体Sniper、5体に1体Tank、残りSpeed/Normal）
        EnemyType type = EnemyType::Normal;
        if (i % 5 == 0) type = EnemyType::Tank;
        else if (i % 3 == 0) type = EnemyType::Sniper;
        else if (i % 2 == 0) type = EnemyType::Speed;

        g_EnemyManager.Spawn(spawns[i], type);
    }

    // プレイヤー追従カメラ
    Player_Camera_Initialize();

    // ビルボード
    Billboard_Initialize();

    HUD_Initialize();

    // プレイヤー初期位置と進行方向（生成マップのスポーンへ）
    Player_Initialize(Map_GetSpawnPosition(), { 0.0f, 0.0f, 1.0f });


    ItemManager_Initialize();
}

//==============================================================================
// 毎フレームの更新処理
//==============================================================================
void Game_Update(double elapsed_time)
{
    // 起動直後・再生成直後の dt 暴走対策
    const double MAX_DT = 1.0 / 30.0; // 33ms
    if (elapsed_time > MAX_DT)
        elapsed_time = MAX_DT;

    //--------------------------------------------------------------------------
    // ダンジョン再生成（Rキー）
    // ・マップを再生成して、プレイヤーを安全スポーン位置へ移動する
    //--------------------------------------------------------------------------
    if (KeyLogger_IsTrigger(KK_R))
    {
        Map_GenerateDungeon(++g_DungeonSeed);
        Map_RegisterFloors();

        Player_SetPosition(Map_GetSpawnPosition(), true);
        Player_ResetHP(); // 　HP回復

        Enemy_Finalize();        // 一旦全削除

        ItemManager_Finalize();   // 既存アイテムを全削除
        ItemManager_Initialize(); // 新しいマップ用に再配置
        const auto& spawns = Map_GetEnemySpawnPositions();
        for (const auto& p : spawns)
        {
            Enemy_Spawn(p);
        }
    }

    // プレイヤーとカメラ、弾、被弾エフェクトの更新
    Player_Update(elapsed_time);
    Player_Camera_Update(elapsed_time);

    EnemyBullet_Update(elapsed_time);
    Bullet_Update(elapsed_time);
    BulletHitEffect_Update();

    HUD_Update(elapsed_time);

    // 追尾エネミー更新
    // エネミー側でプレイヤー衝突判定（ダメージ＋ノックバック）を実施
    //複数種版に変更
    g_EnemyManager.Update(elapsed_time);
    g_EnemyManager.RemoveDead();
    ItemManager_Update();

    // マップオブジェクトと弾の AABB 当たり判定
    for (int j = 0; j < Map_GetObjectsCount(); j++)
    {
        const MapObject* mo = Map_GetObject(j);

        // 床(KindId=0,1)は無視、壁(KindId=2)だけ判定
        if (mo->KindId == 0 || mo->KindId == 1) continue;

        for (int i = 0; i < Bullet_GetCount(); i++)
        {
            AABB bullet = Bullet_GetAABB(i);
            AABB mapObj = mo->Aabb;

            if (Collision_IsOverLapAABB(bullet, mapObj))
            {
                Bullet_Destroy(i);
                i--;  // インデックス調整
                break;
            }
        }
    }
}

//==============================================================================
// 描画処理
//==============================================================================
void Game_Draw()
{
    // ---------------------------------------------------------------------
    // ライティング設定
    // ---------------------------------------------------------------------
    //Light_SetAmbient({ 1.0f, 1.0f, 1.0f });
    //
    //Light_SetDirectionalWorld(
    //    { 0.0f, -1.0f, 0.0f, 0.0f },
    //    { 0.3f,  0.3f,  0.3f, 1.0f }
    //);
    //
    //Light_SetSpecularWorld(
    //    Player_Camera_GetPosition(),
    //    10.0f,
    //    { 0.4f, 0.4f, 0.4f, 1.0f }
    //);

    // ---------------------------------------------------------------------
    // サンプラー設定
    // ---------------------------------------------------------------------
    Sampler_SetFilterAnisotropic();

    // ---------------------------------------------------------------------
    // 丸影（Blob Shadow）
    // ・地面＋キューブに掛けたい
    // ・モデル（Player 等）には掛けたくない
    // → Map_Draw の間だけ有効にして、直後に必ず解除する
    // ---------------------------------------------------------------------
    {
        ID3D11DeviceContext* ctx = Direct3D_GetContext();

        const DirectX::XMFLOAT3 playerPos = Player_GetPosition();
        const AABB playerAabb = Player_GetAABB();

        // キャラの高さ（AABB から算出）
        const float charHeight = (playerAabb.max.y - playerAabb.min.y);

        // 支持面Y（FloorRegistry が未実装なら mapAABB から探索）
        float supportY = 0.0f;
        {
            // 代替：Map AABB から支持面Yを探す
            GetSupportY_FromMapAABBs(playerAabb, &supportY);
        }

        // 足元のY（AABB min.y を足元扱い）
        const float footY = playerAabb.min.y;

        // 支持面からの高さ
        float hLocal = footY - supportY;
        if (hLocal < 0.0f) hLocal = 0.0f;

        // 接地中は見た目高さを 0 固定（床がキューブでも半径が縮まない）
        const float EPS_GROUND = 0.02f;
        const bool onGround = (hLocal <= EPS_GROUND);
        const float visualH = onGround ? 0.0f : hLocal;

        // 自動スケール
        const float baseRadius = charHeight * 0.55f;
        const float baseSoft = baseRadius * 0.55f;
        const float baseStr = 0.45f;

        const float maxH = charHeight * 0.8f;
        float t = (maxH > 0.0001f) ? (visualH / maxH) : 0.0f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        const float radius = baseRadius * (1.0f - 0.45f * t);
        const float softness = baseSoft * (1.0f - 0.15f * t);
        const float strength = baseStr * (1.0f - 0.75f * t);

        // Map（地面＋キューブ）に掛ける
        BlobShadow::SetToPixelShader(ctx, playerPos, radius, softness, strength);

        Map_Draw();

        // ここで必ず解除（これ以降のモデルに影を掛けない）
        ID3D11Buffer* nullCB = nullptr;
        ctx->PSSetConstantBuffers(6, 1, &nullCB); // b6 を使っている前提
    }

    // ---------------------------------------------------------------------
    // 3Dオブジェクト描画（モデル類）
    // ---------------------------------------------------------------------


    // エネミー描画（モデル扱いなので丸影なし側）
    //複数種版に変更
    g_EnemyManager.Draw();
    Player_Draw();

    ItemManager_Draw();

    Bullet_Draw();
    EnemyBullet_Draw();
    BulletHitEffect_Draw();

 

    //==========================================================
    // デバッグ: AABBを可視化
    //==========================================================

    // 壁AABBを赤色で表示
    for (int i = 0; i < Map_GetObjectsCount(); i++)
    {
        const MapObject* mo = Map_GetObject(i);
        if (mo->KindId == 2) // KIND_WALL
        {
            Collision_DebugDraw(mo->Aabb, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤
        }
    }

    // 弾AABBを黄色で表示
    for (int i = 0; i < Bullet_GetCount(); i++)
    {
        AABB bulletAABB = Bullet_GetAABB(i);
        Collision_DebugDraw(bulletAABB, { 1.0f, 1.0f, 0.0f, 1.0f }); // 黄色
    }

    //========================
    // ミニマップ描画

    //========================
    MiniMap_Render3D(); // オフスクリーンに3D描画
    MiniMap_Draw2D();   // 画面に貼る
    //HUD描画
    HUD_Draw();
}

//==============================================================================
// ゲーム終了処理（リソース解放）
//==============================================================================
void Game_Finalize()
{

    ItemManager_Finalize();
    HUD_Finalize();

    Billboard_Finalize();

    // マップ関連
    Map_Finalize();

    // フィールドメッシュ
    MeshField_Finalize();

    // カメラ・プレイヤー
    Player_Camera_Finalize();
    Player_Finalize();

    // エネミー
    g_EnemyManager.Finalize();

    // エフェクト／弾
    BulletHitEffect_Finalize();
    Bullet_Finalize();

    // エネミーバレット
    EnemyBullet_Finalize();

}

