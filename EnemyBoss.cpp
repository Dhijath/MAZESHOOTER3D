/*==============================================================================

   ボス型エネミー [EnemyBoss.cpp]
                                                         Author : 51106
                                                         Date   : 2026/03/09
--------------------------------------------------------------------------------

==============================================================================*/

#include "EnemyBoss.h"
#include "EnemyBullet.h"
#include "MapPatrolAI.h"
#include "Player.h"
#include "Audio.h"
#include "model.h"
#include "ModelToon.h"
#include "Light.h"
#include "Player_Camera.h"
#include <cstdlib>

using namespace DirectX;

//==============================================================================
// 初期化
//
// ■役割
// ・Enemy::Initialize で基本状態をセットアップ
// ・HP・モデル・バレル・SEをボス用に上書きする
//==============================================================================
void EnemyBoss::Initialize(const XMFLOAT3& position)
{
    // 基底クラスの初期化
    Enemy::Initialize(position);

    // HP をボス用に上書き
    SetHP(HP, HP);

    // 本体モデルを差し替え
    ModelRelease(m_pModel);
    m_pModel = ModelLoad("resource/Models/enemy_tank.fbx", BOSS_SIZE);

    // 左右バレルをロード
    m_pBarrelL = ModelLoad("resource/Models/Barrel.fbx", BOSS_SIZE * 0.5f);
    m_pBarrelR = ModelLoad("resource/Models/Barrel.fbx", BOSS_SIZE * 0.5f);

    // ショットガンSEをロード
    m_shootSE = LoadAudioWithVolume("resource/sound/shotgun.wav", 0.5f);
    m_shootTimer = 0.0f;
    m_BarrelRotX = 0.0f;

    // 突進・射撃パラメータ初期化
    m_bossPhase         = BossPhase::NORMAL;
    m_chargeTimer       = 0.0f;
    m_chargeCooldown    = 3.0f;   // 開始直後は少し猶予を持たせる
    m_chargeDir         = {};
    m_chargeDamageDealt = false;
    m_shotCount         = 0;
    m_nextShootInterval = 1.0f;
}

//==============================================================================
// 終了処理
//==============================================================================
void EnemyBoss::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;

    ModelRelease(m_pBarrelL); m_pBarrelL = nullptr;
    ModelRelease(m_pBarrelR); m_pBarrelR = nullptr;

    Enemy::Finalize();
}

//==============================================================================
// 更新処理
//
// ■役割
// ・突進フェーズに応じて速度を事前設定してから基底クラスのUpdateを呼ぶ
// ・基底クラスのUpdate（AI移動・重力・衝突・弾ヒット・死亡判定）を実行
// ・バレルをプレイヤー方向に追従させる
// ・突進ステートマシンを更新する
// ・生存中のみ射撃タイマーを進め、視線が通れば Shoot()/SpreadShoot() を呼ぶ
//==============================================================================
void EnemyBoss::Update(double elapsed_time)
{
    if (!m_IsAlive) return;

    float dt = static_cast<float>(elapsed_time > 1.0 / 30.0 ? 1.0 / 30.0 : elapsed_time);

    //==========================================================================
    // 突進フェーズに応じた速度の事前設定（Enemy::Update 内の EnemyAI に渡す初期値）
    //==========================================================================
    switch (m_bossPhase)
    {
    case BossPhase::CHARGE_WINDUP:
        // 溜め中は停止（EnemyAI が lerp で若干動くが許容範囲）
        m_Velocity.x = 0.0f;
        m_Velocity.z = 0.0f;
        break;

    case BossPhase::CHARGING:
        // 突進方向へ高速移動
        m_Velocity.x = m_chargeDir.x * CHARGE_SPEED;
        m_Velocity.z = m_chargeDir.z * CHARGE_SPEED;
        break;

    case BossPhase::NORMAL:
    {
        // 近づきすぎたら後退（距離を保つ）
        const XMFLOAT3& playerPos = Player_GetPosition();
        float dx    = m_Position.x - playerPos.x;
        float dz    = m_Position.z - playerPos.z;
        float dist2 = dx * dx + dz * dz;
        if (dist2 < KEEP_DISTANCE * KEEP_DISTANCE && dist2 > 0.001f)
        {
            float dist = sqrtf(dist2);
            m_Velocity.x = (dx / dist) * RETREAT_SPEED;
            m_Velocity.z = (dz / dist) * RETREAT_SPEED;
        }
        break;
    }

    default:
        break;
    }

    //==========================================================================
    // 基底クラスのUpdate（AI・移動・衝突・死亡判定）
    //==========================================================================
    Enemy::Update(elapsed_time);

    // 死亡後は以降の処理をスキップ
    if (!m_IsAlive) return;

    //==========================================================================
    // バレル・本体の向き更新
    //==========================================================================
    {
        const XMFLOAT3& playerPos = Player_GetPosition();
        float dx        = playerPos.x - m_Position.x;
        float dy        = (playerPos.y + 0.3f) - m_Position.y;
        float dz        = playerPos.z - m_Position.z;
        float horizDist = sqrtf(dx * dx + dz * dz);

        if (m_bossPhase == BossPhase::CHARGING && (m_chargeDir.x != 0.0f || m_chargeDir.z != 0.0f))
        {
            // 突進中は突進方向に向く
            m_Front      = m_chargeDir;
            m_BarrelRotX = 0.0f;
        }
        else
        {
            // 通常・溜め・クールダウン中はプレイヤー方向を向く
            m_BarrelRotX = atan2f(dy, horizDist);
            if (horizDist > 0.001f)
            {
                m_Front = { dx / horizDist, 0.0f, dz / horizDist };
            }
        }
    }

    //==========================================================================
    // 突進ステートマシン
    //==========================================================================
    switch (m_bossPhase)
    {
    case BossPhase::NORMAL:
    {
        // 突進クールダウンを減算
        m_chargeCooldown -= dt;

        // クールダウン完了 & 視線が通っていれば溜め開始
        if (m_chargeCooldown <= 0.0f &&
            MapPatrolAI_HasLineOfSight(m_Position, Player_GetPosition()))
        {
            m_bossPhase         = BossPhase::CHARGE_WINDUP;
            m_chargeTimer       = 0.0f;
            m_chargeDamageDealt = false;
        }
        break;
    }

    case BossPhase::CHARGE_WINDUP:
    {
        m_chargeTimer += dt;

        // 毎フレーム突進方向をプレイヤー方向に更新（狙いを定める）
        {
            const XMFLOAT3& playerPos = Player_GetPosition();
            float dx  = playerPos.x - m_Position.x;
            float dz  = playerPos.z - m_Position.z;
            float len = sqrtf(dx * dx + dz * dz);
            if (len > 0.001f)
                m_chargeDir = { dx / len, 0.0f, dz / len };
        }

        // 溜め完了 → 突進へ移行
        if (m_chargeTimer >= CHARGE_WINDUP_TIME)
        {
            m_bossPhase   = BossPhase::CHARGING;
            m_chargeTimer = 0.0f;
        }
        break;
    }

    case BossPhase::CHARGING:
    {
        m_chargeTimer += dt;

        // プレイヤー接触チェック（1回のみダメージ）
        if (!m_chargeDamageDealt)
        {
            const XMFLOAT3& playerPos = Player_GetPosition();
            float dx   = playerPos.x - m_Position.x;
            float dz   = playerPos.z - m_Position.z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist < CHARGE_DAMAGE_DIST)
            {
                Player_TakeDamage(CHARGE_DAMAGE);

                // 強ノックバック
                DirectX::XMFLOAT3* pPlayerVel = Player_GetVelocityPtr();
                if (pPlayerVel && dist > 0.001f)
                {
                    pPlayerVel->x += (dx / dist) * CHARGE_KNOCKBACK;
                    pPlayerVel->z += (dz / dist) * CHARGE_KNOCKBACK;
                    pPlayerVel->y += 6.0f;
                }
                m_chargeDamageDealt = true;
            }
        }

        // 突進時間終了 → クールダウンへ移行
        if (m_chargeTimer >= CHARGE_MOVE_TIME)
        {
            m_bossPhase      = BossPhase::COOLDOWN;
            m_chargeTimer    = 0.0f;
            m_chargeCooldown = CHARGE_INTERVAL;
        }
        break;
    }

    case BossPhase::COOLDOWN:
    {
        m_chargeTimer += dt;

        // クールダウン終了 → 通常へ戻る
        if (m_chargeTimer >= CHARGE_COOLDOWN_TIME)
        {
            m_bossPhase   = BossPhase::NORMAL;
            m_chargeTimer = 0.0f;
        }
        break;
    }
    }

    //==========================================================================
    // 射撃タイマー更新（溜め・突進中は射撃しない）
    //==========================================================================
    if (m_bossPhase == BossPhase::NORMAL || m_bossPhase == BossPhase::COOLDOWN)
    {
        m_shootTimer += dt;
        if (m_shootTimer >= m_nextShootInterval)
        {
            m_shootTimer = 0.0f;
            // 次の射撃間隔をランダム化（0.7〜1.6 秒）
            m_nextShootInterval = 0.7f + (rand() % 90) * 0.01f;

            // 視線が通っていれば射撃
            if (MapPatrolAI_HasLineOfSight(m_Position, Player_GetPosition()))
            {
                m_shotCount++;
                if (m_shotCount % 3 == 0)
                    SpreadShoot(); // 3発に1回は散弾
                else
                    Shoot();       // 通常2連射
            }
        }
    }
}

//==============================================================================
// 描画処理
//
// ■役割
// ・本体モデルをトゥーン描画する
// ・左右バレルをプレイヤー追従角度で描画する
//==============================================================================
void EnemyBoss::Draw()
{
    if (!m_IsAlive) return;
    if (!m_pModel)  return;

    Light_SetSpecularWorld(
        Player_Camera_GetPosition(),
        4.0f,
        { 0.8f, 0.2f, 0.2f, 1.0f }  // 赤い光でボスを識別
    );

    float angle = -atan2f(m_Front.z, m_Front.x);

    XMMATRIX rot =
        XMMatrixRotationY(angle) *
        XMMatrixRotationY(XMConvertToRadians(-90.0f));

    XMMATRIX trans = XMMatrixTranslation(
        m_Position.x,
        m_Position.y,
        m_Position.z
    );

    ModelDrawToon(m_pModel, rot * trans);

    // 左右バレル描画
    if (m_pBarrelL && m_pBarrelR)
    {
        ModelDrawToon(m_pBarrelL, GetBarrelWorldMatrix(BARREL_OFFSET));
        ModelDrawToon(m_pBarrelR, GetBarrelWorldMatrix(-BARREL_OFFSET));
    }
}

//==============================================================================
// バレルのワールド行列取得
//
// ■役割
// ・ボス正面方向・仰角・左右オフセットからバレルのワールド行列を返す
// ・Draw() と Shoot() で共用する
//
// ■引数
// ・sideOffset : 正=右バレル / 負=左バレル
//==============================================================================
XMMATRIX EnemyBoss::GetBarrelWorldMatrix(float sideOffset)
{
    float angle = -atan2f(m_Front.z, m_Front.x);

    XMMATRIX barrelRot =
        XMMatrixRotationX(-m_BarrelRotX) *
        XMMatrixRotationY(angle) *
        XMMatrixRotationY(XMConvertToRadians(-90.0f));

    const XMFLOAT3 bodyOrigin = { m_Position.x, m_Position.y, m_Position.z };
    const AABB bodyAABB = ModelGetAABB(m_pModel, bodyOrigin);
    const AABB barrelLocal = ModelGetAABB(m_pBarrelL, { 0.0f, 0.0f, 0.0f });
    const float barrelCenterY = (barrelLocal.max.y + barrelLocal.min.y) * 0.5f;

    // 右方向を計算してsideOffsetでずらす
    XMVECTOR frontV = XMVectorSet(m_Front.x, 0.0f, m_Front.z, 0.0f);
    XMVECTOR rightV = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), frontV));
    XMFLOAT3 right;
    XMStoreFloat3(&right, rightV);

    XMMATRIX barrelTrans = XMMatrixTranslation(
        m_Position.x + m_Front.x * BARREL_FORWARD + right.x * sideOffset,
        bodyAABB.max.y - barrelCenterY,
        m_Position.z + m_Front.z * BARREL_FORWARD + right.z * sideOffset
    );

    return barrelRot * barrelTrans;
}

//==============================================================================
// 射撃処理
//
// ■役割
// ・左右バレルの先端（マズル）位置を取得する
// ・各バレルからプレイヤーの胴体に向けて弾を1発ずつ発射する
// ・発射と同時にショットガンSEを再生する
//==============================================================================
void EnemyBoss::Shoot()
{
    XMFLOAT3 playerPos = Player_GetPosition();
    XMFLOAT3 target = { playerPos.x, playerPos.y + 0.3f, playerPos.z };

    const AABB barrelLocal = ModelGetAABB(m_pBarrelL, { 0.0f, 0.0f, 0.0f });

    // 左右それぞれのバレルから発射
    for (float side : { BARREL_OFFSET, -BARREL_OFFSET })
    {
        XMMATRIX barrelWorld = GetBarrelWorldMatrix(side);

        // バレル先端（ローカルZ最小＝マズル）をワールド座標に変換
        XMVECTOR muzzleLocal = XMVectorSet(0.0f, 0.0f, barrelLocal.min.z, 1.0f);
        XMVECTOR muzzleWorld = XMVector3TransformCoord(muzzleLocal, barrelWorld);

        XMFLOAT3 muzzlePos;
        XMStoreFloat3(&muzzlePos, muzzleWorld);

        XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&target) - muzzleWorld);
        XMFLOAT3 vel;
        XMStoreFloat3(&vel, dir);

        EnemyBullet_Create(muzzlePos, vel, SHOOT_DAMAGE, BOSS_BULLET_SPEED);
    }

    // ショットガンSE再生
    PlayAudio(m_shootSE, false);
}

//==============================================================================
// 散弾処理
//
// ■役割
// ・中央バレル位置から±30°の扇状に5発同時発射する
// ・Shoot() の代わりに3発に1回呼ばれる
//==============================================================================
void EnemyBoss::SpreadShoot()
{
    XMFLOAT3 playerPos = Player_GetPosition();
    XMFLOAT3 target    = { playerPos.x, playerPos.y + 0.3f, playerPos.z };

    // 水平方向の基準角（ボス→プレイヤー）
    float dx      = target.x - m_Position.x;
    float dy      = target.y - m_Position.y;
    float dz      = target.z - m_Position.z;
    float baseYaw = atan2f(dx, dz);  // XZ 平面のヨー角

    // 仰角
    float horizDist    = sqrtf(dx * dx + dz * dz);
    float baseElevation = atan2f(dy, horizDist);
    float cosEl        = cosf(baseElevation);
    float sinEl        = sinf(baseElevation);

    // 中央バレル位置をマズルとして使用
    if (!m_pBarrelL) { Shoot(); return; }
    const AABB barrelLocal = ModelGetAABB(m_pBarrelL, { 0.0f, 0.0f, 0.0f });
    XMMATRIX   barrelCenter = GetBarrelWorldMatrix(0.0f);
    XMVECTOR   muzzleLocal  = XMVectorSet(0.0f, 0.0f, barrelLocal.min.z, 1.0f);
    XMVECTOR   muzzleWorld  = XMVector3TransformCoord(muzzleLocal, barrelCenter);
    XMFLOAT3   muzzlePos;
    XMStoreFloat3(&muzzlePos, muzzleWorld);

    // 扇状5発（±30°, ±15°, 中央）
    const float spreadAngles[] = { -30.0f, -15.0f, 0.0f, 15.0f, 30.0f };
    for (float deg : spreadAngles)
    {
        float a   = baseYaw + XMConvertToRadians(deg);
        XMFLOAT3 vel = {
            sinf(a) * cosEl,
            sinEl,
            cosf(a) * cosEl
        };
        EnemyBullet_Create(muzzlePos, vel, SHOOT_DAMAGE, BOSS_BULLET_SPEED);
    }

    // ショットガンSE再生
    PlayAudio(m_shootSE, false);
}