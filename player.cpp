/*==============================================================================

   プレイヤー制御 [Player.cpp]
                                                         Author : 51106
                                                         Date   : 2026/02/09
--------------------------------------------------------------------------------
==============================================================================*/
#include "Player.h"
#include "model.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "Light.h"
#include "camera.h"
#include "Player_Camera.h"
#include "map.h"
#include "cube.h"
#include "bullet.h"
#include "mouse.h"
#include "collision_obb.h"
#include "Particle.h"
#include "particle_thruster.h"
#include "texture.h"
#include "direct3d.h"
#include "shader3d.h"
#include "meshfield.h"
#include "Audio.h"
#include "HUD.h"
#include "score.h"
using namespace DirectX;

namespace
{
    //--------------------------------------------------------------------------
    // プレイヤー状態
    //--------------------------------------------------------------------------
    XMFLOAT3 g_PlayerPosition = {};
    XMFLOAT3 g_PlayerStartPosition = {};
    XMFLOAT3 g_PlayerFront = { 0.0f, 0.0f, 1.0f };
    XMFLOAT3 g_PlayerVelocity = {};
    MODEL* g_pPlayerModel = nullptr;
    bool g_IsJump = false;
    bool g_PlayerEnable = true;

    float g_PlayerSpeedMultiplier = 1.0f;
    int g_PlayerWhightTexID = -1;        // プレイヤー矢印テクスチャ

    //--------------------------------------------------------------------------
    // HP / 無敵
    //--------------------------------------------------------------------------
    constexpr int PLAYER_MAX_HP = 100;
    int g_PlayerHP = PLAYER_MAX_HP;      // 現在HP
    double g_InvincibleTimer = 0.0;
    constexpr double INVINCIBLE_DURATION = 1.3; // 無敵時間（秒）

    //--------------------------------------------------------------------------
    // 当たり判定サイズ
    //--------------------------------------------------------------------------
    const float PLAYER_HEIGHT = 0.10f / 2.0f;
    const float PLAYER_HALF_WIDTH_X = 0.5f / 2.0f;
    const float PLAYER_HALF_WIDTH_Z = 0.75f / 2.0f;

    //--------------------------------------------------------------------------
    // 火器関連ステータス
    //--------------------------------------------------------------------------
    bool   g_IsBeamMode = false;                    // 武器モード（true=ビーム / false=通常弾）
    double g_BeamCooldown = 0.0;                    // ビームの発射間隔タイマー（秒）
    constexpr double BEAM_FIRE_INTERVAL = 0.001;    // ビームの発射間隔（秒）
    float g_PlayerDamageMultiplier = 1.0f;          // プレイヤーの攻撃力倍率（強化システム用）
    constexpr float BEAM_ENERGY_MAX = 300.0f;       // ビームエネルギー最大値
    constexpr float BEAM_ENERGY_COST = 1.0f;        // ビーム1発のエネルギーコスト
    float g_BeamEnergy = BEAM_ENERGY_MAX;           // 現在のビームエネルギー

    //--------------------------------------------------------------------------
    // SE関連
    //--------------------------------------------------------------------------
    int g_PlayerShootSE = -1;  // 通常弾射撃SE
    int g_PlayerModeSwitchToBeamSE = -1;  // ビームモード切り替えSE
    int g_PlayerModeSwitchToNormalSE = -1;  // 通常弾モード切り替えSE
    int g_PlayerBeamShootSE = -1;  // ビーム発射SE

    double g_BeamShootSECooldown = 0.0;            // ビーム発射SEのクールダウンタイマー（秒）
    constexpr double BEAM_SHOOT_SE_INTERVAL = 0.1; // ビーム発射SEの再生間隔（秒）

    //--------------------------------------------------------------------------
    // パーティクル（スラスター）
    //--------------------------------------------------------------------------
    int g_PlayerParticleTexID = -1;                     // スラスター用テクスチャID
    ThrusterEmitter* g_PlayerThrusterEmitter = nullptr; // 後方噴射用エミッター

    // ローカルオフセット（right/up/front 基底で組み立て）
    // X: 右+ / 左-  Y: 上+ / 下-  Z: 前+ / 後-
    const XMFLOAT3 g_ThrusterOffsetLocal = { 0.0f, 0.30f, -0.25f };

    //--------------------------------------------------------------------------
    // 壁押し戻し（現在位置での OBB vs 壁AABB を使い、押し戻しベクトルを決定）
    //--------------------------------------------------------------------------
    static bool ResolveWallCollisionAtPosition(XMVECTOR* ioPos, XMVECTOR* ioVel) // 壁(KindId=2)との衝突を解決し、押し戻しで位置/速度を補正する。ioPos=位置(入出力) ioVel=速度(入出力) 戻り値=trueで衝突あり
    {
        XMFLOAT3 currentPos;
        XMStoreFloat3(&currentPos, *ioPos);

        float maxPenetration = 0.0f;
        XMFLOAT3 bestSeparation = { 0.0f, 0.0f, 0.0f };
        bool foundCollision = false;

        for (int i = 0; i < Map_GetObjectsCount(); i++)
        {
            const MapObject* mo = Map_GetObject(i);
            if (!mo) continue;
            if (mo->KindId != 2) continue;

            OBB playerOBB = Player_ConvertPositionToOBB(*ioPos);
            Hit hit = Collision_IsHitOBB_AABB(playerOBB, mo->Aabb);
            if (!hit.isHit) continue;

            const AABB& wall = mo->Aabb;
            float separationX = 0.0f;
            float separationZ = 0.0f;

            float playerLeft = currentPos.x - PLAYER_HALF_WIDTH_X;
            float playerRight = currentPos.x + PLAYER_HALF_WIDTH_X;

            if (playerRight > wall.min.x && playerLeft < wall.max.x)
            {
                float pushLeft = wall.min.x - playerRight;
                float pushRight = wall.max.x - playerLeft;
                separationX = (fabsf(pushLeft) < fabsf(pushRight)) ? pushLeft : pushRight;
            }

            float playerNear = currentPos.z - PLAYER_HALF_WIDTH_Z;
            float playerFar = currentPos.z + PLAYER_HALF_WIDTH_Z;

            if (playerFar > wall.min.z && playerNear < wall.max.z)
            {
                float pushNear = wall.min.z - playerFar;
                float pushFar = wall.max.z - playerNear;
                separationZ = (fabsf(pushNear) < fabsf(pushFar)) ? pushNear : pushFar;
            }

            float absX = fabsf(separationX);
            float absZ = fabsf(separationZ);

            if (absX > 0.0001f || absZ > 0.0001f)
            {
                float penetration = (absX < absZ) ? absX : absZ;

                if (penetration > maxPenetration)
                {
                    maxPenetration = penetration;
                    foundCollision = true;

                    if (absX < absZ)
                        bestSeparation = { separationX, 0.0f, 0.0f };
                    else
                        bestSeparation = { 0.0f, 0.0f, separationZ };
                }
            }
        }

        if (foundCollision)
        {
            *ioPos = XMVectorAdd(*ioPos, XMLoadFloat3(&bestSeparation));

            XMFLOAT3 vel;
            XMStoreFloat3(&vel, *ioVel);

            const float restitution = -0.1f;

            if (fabsf(bestSeparation.x) > 0.0001f)
                vel.x *= restitution;
            if (fabsf(bestSeparation.z) > 0.0001f)
                vel.z *= restitution;

            *ioVel = XMLoadFloat3(&vel);
        }

        return foundCollision;

    }
    //--------------------------------------------------------------------------
    // ビームモードの状態遷移フラグの管理
    //--------------------------------------------------------------------------

    static void Player_SetBeamMode(bool toBeam, bool playSE, bool notifyHUD)
    {
        if (g_IsBeamMode == toBeam) return;

        g_IsBeamMode = toBeam;

        if (playSE)
        {
            const int switchSE = g_IsBeamMode ? g_PlayerModeSwitchToBeamSE : g_PlayerModeSwitchToNormalSE;
            if (switchSE >= 0) PlayAudio(switchSE, false);
        }

        if (notifyHUD)
        {
            HUD_NotifyModeChange(g_IsBeamMode);
        }
    }




    //--------------------------------------------------------------------------
    // 速度が速いときのすり抜け対策：小ステップに分割して移動＋衝突解決を繰り返す
    //--------------------------------------------------------------------------
    static void MoveWithSubSteps(XMVECTOR* ioPos, XMVECTOR* ioVel, float dt) // すり抜け防止の分割移動。ioPos=位置(入出力) ioVel=速度(入出力) dt=このフレームの経過時間(秒)
    {
        const float maxStep = 0.05f;
        const XMVECTOR delta0 = (*ioVel) * dt;

        const float dx = fabsf(XMVectorGetX(delta0));
        const float dy = fabsf(XMVectorGetY(delta0));
        const float dz = fabsf(XMVectorGetZ(delta0));

        const float len3 = sqrtf(dx * dx + dy * dy + dz * dz);

        int steps = (int)ceilf(len3 / maxStep);
        if (steps < 1)   steps = 1;
        if (steps > 128) steps = 128;

        const float dtStep = dt / (float)steps;

        int collisionCount = 0;
        const int maxCollisions = 3;

        for (int s = 0; s < steps; ++s)
        {
            *ioPos += (*ioVel) * dtStep;

            bool hit = ResolveWallCollisionAtPosition(ioPos, ioVel);

            if (hit)
            {
                collisionCount++;

                if (collisionCount >= maxCollisions)
                {
                    *ioVel = XMVectorSetX(*ioVel, 0.0f);
                    *ioVel = XMVectorSetZ(*ioVel, 0.0f);
                    break;
                }
            }
        }
    }
}

//==============================================================================
// 初期化
//==============================================================================
void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3 front) // プレイヤーの初期化（位置/向き/HP/入力/モデル/スラスター生成）。position=初期位置 front=初期正面方向
{
    g_PlayerPosition = position;
    g_PlayerStartPosition = position;
    g_PlayerVelocity = { 0.0f, 0.0f, 0.0f };
    g_PlayerEnable = true;

    g_PlayerDamageMultiplier = 1.0f;
    g_PlayerSpeedMultiplier = 1.0f;


    g_PlayerHP = PLAYER_MAX_HP;
    g_BeamEnergy = BEAM_ENERGY_MAX;
    g_InvincibleTimer = 0.0;

    Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
    Mouse_SetVisible(false);

    XMStoreFloat3(&g_PlayerFront, XMVector3Normalize(XMLoadFloat3(&front)));

    g_PlayerWhightTexID = Texture_Load(L"resource/texture/Player_white.png");
    g_pPlayerModel = ModelLoad("resource/Models/E 45 Aircraft_obj.obj", 0.08);

    //--------------------------------------------------------------------------
    // SE読み込み
    //--------------------------------------------------------------------------
    g_PlayerShootSE = LoadAudioWithVolume("resource/sound/maou_se_battle_gun05.wav", 0.5f); // 通常弾射撃SE
    g_PlayerModeSwitchToBeamSE = LoadAudioWithVolume("resource/sound/mode_switch_beam.wav", 0.5f); // ビームモード切り替えSE
    g_PlayerModeSwitchToNormalSE = LoadAudioWithVolume("resource/sound/mode_switch_normal.wav", 0.5f); // 通常弾モード切り替えSE
    g_PlayerBeamShootSE = LoadAudioWithVolume("resource/sound/beam_shoot.wav", 0.5f); // ビーム発射SE

    PadLogger_Initialize();

    //--------------------------------------------------------------------------
    // パーティクル初期化（スラスター）
    //--------------------------------------------------------------------------
    g_PlayerParticleTexID = Texture_Load(L"resource/texture/effect000.jpg");

    XMVECTOR playerVec = XMLoadFloat3(&g_PlayerPosition);

    g_PlayerThrusterEmitter = new ThrusterEmitter(playerVec, 320.0, true);
    g_PlayerThrusterEmitter->SetParticleTextureId(g_PlayerParticleTexID);

    // 見た目パラメータ（ここを調整して表現を作る）
    g_PlayerThrusterEmitter->SetScaleRange(0.0005f, 0.15f);    // パーティクルのスケール範囲（最小, 最大）
    g_PlayerThrusterEmitter->SetSpeedRange(2.0f, 6.0f);         // パーティクルの速度範囲（最小, 最大）
    g_PlayerThrusterEmitter->SetLifeRange(0.18f, 0.45f);        // パーティクルの寿命範囲（最小, 最大）秒
    g_PlayerThrusterEmitter->SetConeAngleDeg(18.0f);            // 放出コーン角度（度）
    g_PlayerThrusterEmitter->SetColor({ 1.0f, 0.5f, 2.5f, 1.0f }); // パーティクルの色（R,G,B,A）
    g_PlayerThrusterEmitter->SetUVRect({ 0.0f, 0.0f, 80.0f, 80.0f }); // UV矩形（x, y, 幅, 高さ）
    g_PlayerThrusterEmitter->SetLocalOffset(g_ThrusterOffsetLocal); // エミッターのローカルオフセット座標
}

//==============================================================================
// 終了
//==============================================================================
void Player_Finalize() // プレイヤーの終了処理（モデル解放・スラスター破棄・SE解放）
{
    ModelRelease(g_pPlayerModel);

    if (g_PlayerThrusterEmitter)
    {
        delete g_PlayerThrusterEmitter;
        g_PlayerThrusterEmitter = nullptr;
    }

    //--------------------------------------------------------------------------
    // SE解放
    //--------------------------------------------------------------------------
    UnloadAudio(g_PlayerShootSE);            // 通常弾射撃SE解放
    UnloadAudio(g_PlayerModeSwitchToBeamSE);   // ビームモード切り替えSE解放
    UnloadAudio(g_PlayerModeSwitchToNormalSE); // 通常弾モード切り替えSE解放
    UnloadAudio(g_PlayerBeamShootSE);        // ビーム発射SE解放

    g_PlayerShootSE = -1;
    g_PlayerModeSwitchToBeamSE = -1;
    g_PlayerModeSwitchToNormalSE = -1;
    g_PlayerBeamShootSE = -1;
}

//==============================================================================
// 更新
//==============================================================================
void Player_Update(double elapsed_time)
{
    if (elapsed_time > (1.0 / 30.0)) elapsed_time = (1.0 / 30.0);

    if (g_InvincibleTimer > 0.0)
    {
        g_InvincibleTimer -= elapsed_time;
        if (g_InvincibleTimer < 0.0) g_InvincibleTimer = 0.0;
    }

    if (!g_PlayerEnable)
        return;

    if (KeyLogger_IsTrigger(KK_P))
    {
        g_PlayerPosition = g_PlayerStartPosition;
        g_PlayerVelocity = { 0.0f, 0.0f, 0.0f };
        g_IsJump = false;
        g_PlayerPosition.y += 0.002f;
    }

    XMVECTOR position = XMLoadFloat3(&g_PlayerPosition);
    XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);
    XMVECTOR gravityVelocity = XMVectorZero();

    const bool padJump = PadLogger_IsTrigger(PAD_A);

    if ((KeyLogger_IsTrigger(KK_F) || padJump) && !g_IsJump)
    {
        velocity += XMVECTOR{ 0.0f, 10.0f, 0.0f, 0.0f };
        g_IsJump = true;
    }

    XMVECTOR gravityDir = XMVECTOR{ 0.0f, -1.0f, 0.0f, 0.0f };
    velocity += gravityDir * 9.8f * 3.0f * static_cast<float>(elapsed_time);

    gravityVelocity = velocity * static_cast<float>(elapsed_time);

    {
        XMVECTOR posY = position + XMVectorSet(0.0f, XMVectorGetY(gravityVelocity), 0.0f, 0.0f);

        XMFLOAT3 tempPos;
        XMStoreFloat3(&tempPos, posY);

        AABB playerAABB =
        {
            {
                tempPos.x - PLAYER_HALF_WIDTH_X,
                tempPos.y,
                tempPos.z - PLAYER_HALF_WIDTH_Z
            },
            {
                tempPos.x + PLAYER_HALF_WIDTH_X,
                tempPos.y + PLAYER_HEIGHT,
                tempPos.z + PLAYER_HALF_WIDTH_Z
            }
        };

        for (int i = 0; i < Map_GetObjectsCount(); i++)
        {
            const MapObject* mo = Map_GetObject(i);
            if (!mo) continue;
            if (mo->KindId != 0 && mo->KindId != 1) continue;

            if (Collision_IsOverLapAABB(playerAABB, mo->Aabb))
            {
                posY = XMVectorSetY(posY, mo->Aabb.max.y);
                velocity *= XMVECTOR{ 1.0f, 0.0f, 1.0f, 1.0f };
                g_IsJump = false;
                break;
            }
        }

        position = XMVectorSetY(position, XMVectorGetY(posY));
    }

    XMVECTOR moveDir = XMVectorZero();

    XMFLOAT3 camFront = Player_Camera_GetFront();

    XMVECTOR front = XMVector3Normalize(XMVectorSet(camFront.x, 0.0f, camFront.z, 0.0f));
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), front));

    if (KeyLogger_IsPressed(KK_W)) moveDir += front;
    if (KeyLogger_IsPressed(KK_S)) moveDir -= front;
    if (KeyLogger_IsPressed(KK_D)) moveDir += right;
    if (KeyLogger_IsPressed(KK_A)) moveDir -= right;

    float lx = 0.0f, ly = 0.0f;
    PadLogger_GetLeftStick(&lx, &ly);

    moveDir += front * ly;
    moveDir += right * lx;

    if (XMVectorGetX(XMVector3LengthSq(moveDir)) > 0.0f)
    {
        moveDir = XMVector3Normalize(moveDir);

        float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_PlayerFront), moveDir));
        float angle = acosf(std::clamp(dot, -1.0f, 1.0f));

        const float ROT_SPEED = XM_2PI * 2.0f * static_cast<float>(elapsed_time);

        if (angle < ROT_SPEED)
        {
            front = moveDir;
        }
        else
        {
            XMMATRIX r = XMMatrixIdentity();

            if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_PlayerFront), moveDir)) < 0.0f)
                r = XMMatrixRotationY(-ROT_SPEED);
            else
                r = XMMatrixRotationY(ROT_SPEED);

            front = XMVector3TransformNormal(XMLoadFloat3(&g_PlayerFront), r);
        }

        velocity += front * static_cast<float>(2000.0 / 90.0 * elapsed_time);

        XMStoreFloat3(&g_PlayerFront, front);
    }

    velocity += -velocity * static_cast<float>(4.0f * elapsed_time);

    MoveWithSubSteps(&position, &velocity, static_cast<float>(elapsed_time));

    XMStoreFloat3(&g_PlayerPosition, position);
    XMStoreFloat3(&g_PlayerVelocity, velocity);

    //--------------------------------------------------------------------------
    // 武器切り替え（KK_Q キーまたはパッド Bボタン）
    //--------------------------------------------------------------------------
    if (KeyLogger_IsTrigger(KK_Q) || PadLogger_IsTrigger(PAD_LEFT_SHOULDER))
    {
        Player_SetBeamMode(!g_IsBeamMode, /*playSE=*/true, /*notifyHUD=*/true);
    }


    // エネルギー不足なら通常弾モードに強制切り替え
    if (g_BeamEnergy <= 0.0f)
    {
        // 切替ルートを通す
        Player_SetBeamMode(false, /*playSE=*/true, /*notifyHUD=*/true);
    }

    //--------------------------------------------------------------------------
    // 発射ボタン判定（モードによって押しっぱなし/単発を切り替え）
    //--------------------------------------------------------------------------
    const bool padAttack = g_IsBeamMode
        ? PadLogger_IsPressed(PAD_RIGHT_SHOULDER)
        : PadLogger_IsTrigger(PAD_RIGHT_SHOULDER);

    const bool mouseAttack = g_IsBeamMode
        ? Player_Camera_IsMouseLeftPressed()
        : Player_Camera_IsMouseLeftTrigger();

    const bool keyAttack = g_IsBeamMode
        ? KeyLogger_IsPressed(KK_SPACE)
        : KeyLogger_IsTrigger(KK_SPACE);

    //--------------------------------------------------------------------------
    // ビームクールダウン更新
    //--------------------------------------------------------------------------
    if (g_BeamCooldown > 0.0)
        g_BeamCooldown -= elapsed_time;

    // ビーム発射SEのクールダウン更新
    if (g_BeamShootSECooldown > 0.0)
        g_BeamShootSECooldown -= elapsed_time;

    //--------------------------------------------------------------------------
    // 発射処理
    //--------------------------------------------------------------------------
    if (keyAttack || mouseAttack || padAttack)
    {
        XMFLOAT3 camFrontShoot = Player_Camera_GetFront();

        XMVECTOR vCamFrontXZ = XMVector3Normalize(
            XMVectorSet(camFrontShoot.x, 0.0f, camFrontShoot.z, 0.0f)
        );

        XMStoreFloat3(&g_PlayerFront, vCamFrontXZ);

        XMVECTOR vPos = XMLoadFloat3(&g_PlayerPosition);

        XMVECTOR vShootPos = vPos
            + XMVectorSet(0.0f, 0.25f, 0.0f, 0.0f)
            + (vCamFrontXZ * 0.05f);

        XMFLOAT3 shoot_pos, b_velocity;
        XMStoreFloat3(&shoot_pos, vShootPos);

        if (g_IsBeamMode)
        {
            if (g_BeamCooldown <= 0.0 && g_BeamEnergy >= BEAM_ENERGY_COST)
            {
                const int baseDamage = 4; // ビームの基礎ダメージ
                const int finalDamage = static_cast<int>(baseDamage * g_PlayerDamageMultiplier);

                XMStoreFloat3(&b_velocity, vCamFrontXZ);
                Bullet_CreateBeam(shoot_pos, b_velocity, finalDamage);
                g_BeamCooldown = BEAM_FIRE_INTERVAL;

                g_BeamEnergy -= BEAM_ENERGY_COST;
                if (g_BeamEnergy < 0.0f) g_BeamEnergy = 0.0f;

                // SEクールダウンが切れていたら再生（連射時の音の重複を防ぐ）
                if (g_BeamShootSECooldown <= 0.0)
                {
                    PlayAudio(g_PlayerBeamShootSE, false);
                    g_BeamShootSECooldown = BEAM_SHOOT_SE_INTERVAL;
                }
            }
        }
        else
        {
            const int baseDamage = 100; // 通常弾の基礎ダメージ
            const int finalDamage = static_cast<int>(baseDamage * g_PlayerDamageMultiplier);

            XMStoreFloat3(&b_velocity, vCamFrontXZ * 20.0f); // 弾速20m/s
            Bullet_Create(shoot_pos, b_velocity, finalDamage);
            PlayAudio(g_PlayerShootSE, false); // 通常弾射撃SE再生
        }
    }

    //--------------------------------------------------------------------------
    // スラスターの外観をスピード倍率に応じて変化させる
    //--------------------------------------------------------------------------
    {
        // スピード倍率(1.0〜2.0)を0〜1に正規化した補間係数
        float t = std::clamp((g_PlayerSpeedMultiplier - 1.0f) / 1.0f, 0.0f, 1.0f);

        // 色：緑(t=0.0) → 青(t=0.33) → 紫(t=0.66) → 赤(t=1.0)
        float r, g, b;
        if (t < 0.33f)
        {
            float s = t / 0.33f;

            r = 1.5f * s;
            g = 1.0f;
            b = 2.5f;
        }
        else if (t < 0.66f)
        {
            float s = (t - 0.33f) / 0.33f;
            r = 1.0f;
            g = 2.5f * (1.0f - s);
            b = 2.5f * s;

            r = 2.5f;
            g = 1.0f;
            b = 2.5f * (1.0f - s);
        }
        else
        {
            float s = (t - 0.66f) / 0.34f;
            r = 1.0f;
            g = 2.5f * (1.0f - s);
            b = 2.5f * s;
        }
        g_PlayerThrusterEmitter->SetColor({ r, g, b, 1.0f });



        // スケール：倍率が上がるほど小さくなる（下限 0.0001f）
        float scaleMin = std::max(0.0005f - t * 0.0004f, 0.0001f);
        float scaleMax = std::max(0.15f + t * 0.10f, 0.05f);
        g_PlayerThrusterEmitter->SetScaleRange(scaleMin, scaleMax);

        // 寿命：倍率が上がるほど長くなり、量が増えて見える
        float lifeMin = 0.18f + t * 0.30f;
        float lifeMax = 0.45f + t * 0.80f;
        g_PlayerThrusterEmitter->SetLifeRange(lifeMin, lifeMax);
    }

    //--------------------------------------------------------------------------
    // パーティクル更新（スラスター）
    //--------------------------------------------------------------------------
    if (g_PlayerThrusterEmitter)
    {
        bool isMoveInput = false;

        if (KeyLogger_IsPressed(KK_W)) isMoveInput = true;
        if (KeyLogger_IsPressed(KK_S)) isMoveInput = true;
        if (KeyLogger_IsPressed(KK_A)) isMoveInput = true;
        if (KeyLogger_IsPressed(KK_D)) isMoveInput = true;

        float lx2 = 0.0f, ly2 = 0.0f;
        PadLogger_GetLeftStick(&lx2, &ly2);
        if (fabsf(lx2) > 0.1f || fabsf(ly2) > 0.1f)
            isMoveInput = true;

        g_PlayerThrusterEmitter->Emmit(isMoveInput);

        XMVECTOR pos = XMLoadFloat3(&g_PlayerPosition);
        XMVECTOR fwd = XMVector3Normalize(XMLoadFloat3(&g_PlayerFront));

        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR rightW = XMVector3Normalize(XMVector3Cross(up, fwd));

        XMVECTOR off =
            rightW * g_ThrusterOffsetLocal.x +
            up * g_ThrusterOffsetLocal.y +
            fwd * g_ThrusterOffsetLocal.z;

        XMVECTOR emitPos = pos + off;

        XMFLOAT3 backDir{};
        XMStoreFloat3(&backDir, XMVector3Normalize(-fwd));

        g_PlayerThrusterEmitter->SetPosition(emitPos);
        g_PlayerThrusterEmitter->SetWorldDirection(backDir);
        g_PlayerThrusterEmitter->SetWorldUp({ 0.0f, 1.0f, 0.0f });

        g_PlayerThrusterEmitter->Update(elapsed_time);
    }
}

//==============================================================================
// 描画
//==============================================================================
void Player_Draw() // プレイヤー描画（無敵点滅の考慮、モデル描画、スラスター描画）
{
    if (!g_PlayerEnable) return;

    if (g_InvincibleTimer > 0.0)
    {
        int cycle = static_cast<int>(g_InvincibleTimer * 10.0);
        if (cycle % 2 == 0) return;
    }

    Light_SetSpecularWorld(
        Player_Camera_GetPosition(),
        4.0f,
        { 0.6f, 0.5f, 0.4f, 1.0f } // プレイヤの光沢設定
    );

    float angle = -atan2f(g_PlayerFront.z, g_PlayerFront.x) + XMConvertToRadians(0.0f);
    const float pitchDeg = 0.0f;
    const float rollDeg = 0.0f;

    XMMATRIX rotFix = XMMatrixRotationZ(XMConvertToRadians(rollDeg)) *
        XMMatrixRotationX(XMConvertToRadians(pitchDeg));
    XMMATRIX rotYawFix = XMMatrixRotationY(XMConvertToRadians(90.0f));
    XMMATRIX rotY = XMMatrixRotationY(angle);
    XMMATRIX rot = rotFix * rotY * rotYawFix;

    const float heightOffset = 0.25f;

    XMMATRIX t = XMMatrixTranslation(
        g_PlayerPosition.x,
        g_PlayerPosition.y + heightOffset,
        g_PlayerPosition.z
    );

    XMMATRIX world = rot * t;
    ModelDraw(g_pPlayerModel, world);

    if (g_PlayerThrusterEmitter)
    {
        g_PlayerThrusterEmitter->Draw();
    }

    // ミニマップ用プレイヤーマーカー（赤い四角）
    {
        ID3D11DeviceContext* ctx = Direct3D_GetContext();

        // 丸影を一時的に無効化
        ID3D11Buffer* nullCB = nullptr;
        ctx->PSSetConstantBuffers(6, 1, &nullCB);

        const float minimapY = 10.0f;

        Shader3d_Begin();
        Shader3d_SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

        const XMMATRIX markerWorld = XMMatrixTranslation(
            g_PlayerPosition.x,
            minimapY + 1.0f,
            g_PlayerPosition.z
        );

        MeshField_DrawTile(markerWorld, Map_GetWiteTexID(), 1.0f);
        Shader3d_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }
}

//==============================================================================
// 当たり判定取得
//==============================================================================
OBB Player_GetOBB() // 現在のプレイヤー状態から OBB（向き付き当たり判定）を生成して返す
{
    XMFLOAT3 halfExtents = { PLAYER_HALF_WIDTH_X, PLAYER_HEIGHT * 0.5f, PLAYER_HALF_WIDTH_Z };

    XMFLOAT3 center = {
        g_PlayerPosition.x,
        g_PlayerPosition.y + PLAYER_HEIGHT * 0.5f,
        g_PlayerPosition.z
    };

    return OBB::CreateFromFront(center, halfExtents, g_PlayerFront);
}

OBB Player_ConvertPositionToOBB(const DirectX::XMVECTOR& position) // 指定位置positionを中心としてプレイヤーOBBを生成して返す（予測衝突などに使用）。position=OBB中心算出の基準位置
{
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, position);

    XMFLOAT3 halfExtents = { PLAYER_HALF_WIDTH_X, PLAYER_HEIGHT * 0.5f, PLAYER_HALF_WIDTH_Z };

    XMFLOAT3 center = {
        pos.x,
        pos.y + PLAYER_HEIGHT * 0.5f,
        pos.z
    };

    return OBB::CreateFromFront(center, halfExtents, g_PlayerFront);
}

AABB Player_GetAABB() // 現在のプレイヤー位置から AABB（軸揃え当たり判定）を生成して返す（床判定などに使用）
{
    return {
        {
            g_PlayerPosition.x - PLAYER_HALF_WIDTH_X,
            g_PlayerPosition.y,
            g_PlayerPosition.z - PLAYER_HALF_WIDTH_Z
        },
        {
            g_PlayerPosition.x + PLAYER_HALF_WIDTH_X,
            g_PlayerPosition.y + PLAYER_HEIGHT,
            g_PlayerPosition.z + PLAYER_HALF_WIDTH_Z
        }
    };
}

//==============================================================================
// ゲッター
//==============================================================================
const DirectX::XMFLOAT3& Player_GetPosition() // プレイヤー現在位置の参照を返す
{
    return g_PlayerPosition;
}

const DirectX::XMFLOAT3& Player_GetFront() // プレイヤー正面方向ベクトルの参照を返す
{
    return g_PlayerFront;
}

float Player_GetHalfWidthX() // 当たり判定のX方向半幅を返す
{
    return PLAYER_HALF_WIDTH_X;
}

float Player_GetHalfWidthZ() // 当たり判定のZ方向半幅を返す
{
    return PLAYER_HALF_WIDTH_Z;
}

float Player_GetHeight() // 当たり判定の高さを返す
{
    return PLAYER_HEIGHT;
}

bool Player_IsEnable() // プレイヤー操作/更新が有効かどうかを返す
{
    return g_PlayerEnable;
}

//==============================================================================
// セッター
//==============================================================================
void Player_SetEnable(bool enable) // プレイヤー操作/更新の有効無効を切り替える。enable=trueで有効 falseで無効
{
    g_PlayerEnable = enable;

    if (!g_PlayerEnable)
    {
        g_PlayerVelocity = { 0.0f, 0.0f, 0.0f };
        g_IsJump = false;
    }
}

void Player_SetPosition(const DirectX::XMFLOAT3& position, bool setAsStart) // プレイヤー位置を強制設定する。position=設定位置 setAsStart=trueで開始位置も更新
{
    g_PlayerPosition = position;

    if (setAsStart)
        g_PlayerStartPosition = position;

    g_PlayerVelocity = { 0.0f, 0.0f, 0.0f };
    g_IsJump = false;
    g_PlayerEnable = true;
}

DirectX::XMFLOAT3* Player_GetVelocityPtr() // プレイヤー速度ベクトルのポインタを返す（外部から速度を書き換える用途）
{
    return &g_PlayerVelocity;
}

//==============================================================================
// ダメージ / HP
//==============================================================================
bool Player_TakeDamage(int damage) // ダメージ処理（無敵中は無効、HP減算、無敵時間付与、HP0で無効化）。damage=受けるダメージ量 戻り値=trueでダメージ適用
{
    if (g_InvincibleTimer > 0.0 || !g_PlayerEnable)
        return false;

    g_PlayerHP -= damage;

    g_InvincibleTimer = INVINCIBLE_DURATION;

    if (g_PlayerHP <= 0)
    {
        g_PlayerHP = 0;
        Player_SetEnable(false);
    }

    return true;
}

int Player_GetHP() // 現在HPを返す
{
    return g_PlayerHP;
}

int Player_GetMaxHP() // 最大HPを返す
{
    return PLAYER_MAX_HP;
}

void Player_Heal(int amount) // HP回復（最大HPでクランプ）。amount=回復量
{
    g_PlayerHP += amount;
    if (g_PlayerHP > PLAYER_MAX_HP)
        g_PlayerHP = PLAYER_MAX_HP;
}

void Player_ResetHP() // HPと無敵時間を初期状態に戻す
{
    g_PlayerHP = PLAYER_MAX_HP;
    g_InvincibleTimer = 0.0;
}

bool Player_IsInvincible() // 無敵中かどうかを返す（trueで無敵）
{
    return g_InvincibleTimer > 0.0;
}

//==============================================================================
// 攻撃力倍率取得
//==============================================================================
float Player_GetDamageMultiplier() // 攻撃力倍率を返す
{
    return g_PlayerDamageMultiplier;
}

//==============================================================================
// 攻撃力倍率設定
//==============================================================================
void Player_SetDamageMultiplier(float multiplier) // 攻撃力倍率を設定する。multiplier=設定する倍率
{
    g_PlayerDamageMultiplier = multiplier;
}

//==============================================================================
// ビームエネルギー取得
//==============================================================================
float Player_GetBeamEnergy() // 現在のビームエネルギーを返す
{
    return g_BeamEnergy;
}

//==============================================================================
// ビームエネルギー最大値取得
//==============================================================================
float Player_GetBeamEnergyMax() // ビームエネルギーの最大値を返す
{
    return BEAM_ENERGY_MAX;
}

//==============================================================================
// ビームエネルギー回復
//==============================================================================
void Player_AddBeamEnergy(float amount) // ビームエネルギーを回復する（最大値でクランプ）。amount=回復量
{
    g_BeamEnergy += amount;
    if (g_BeamEnergy > BEAM_ENERGY_MAX)
        g_BeamEnergy = BEAM_ENERGY_MAX;
}

float Player_GetSpeedMultiplier() // スピード倍率を返す
{
    return g_PlayerSpeedMultiplier;
}

void Player_SetSpeedMultiplier(float m) // スピード倍率を設定する。m=設定する倍率
{
    g_PlayerSpeedMultiplier = m;
}