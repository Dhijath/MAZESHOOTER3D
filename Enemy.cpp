/*==============================================================================

   エネミー制御 [Enemy.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/16
--------------------------------------------------------------------------------

==============================================================================*/

#include "Enemy.h"
#include "model.h"
#include "map.h"
#include "Player.h"
#include "bullet.h"
#include "Light.h"
#include "Player_Camera.h"
#include "collision_obb.h"
#include "EnemyAI.h"
#include "MapPatrolAI.h"
#include <algorithm>
#include <cmath>
#include "Audio.h"
#include "score.h"
#include "ItemManager.h"

using namespace DirectX;

namespace
{
    int g_enemy_hitSE = -1;
    int g_enemy_deadSE = -1;
}

void Enemy_LoadSE()
{
    if (g_enemy_hitSE < 0)
        g_enemy_hitSE = LoadAudioWithVolume("resource/sound/hit.wav", 0.2f);

    if (g_enemy_deadSE < 0)
        g_enemy_deadSE = LoadAudioWithVolume("resource/sound/dead.wav", 1.0f);
}

void Enemy_UnloadSE()
{
    if (g_enemy_hitSE >= 0) { UnloadAudio(g_enemy_hitSE);  g_enemy_hitSE = -1; }
    if (g_enemy_deadSE >= 0) { UnloadAudio(g_enemy_deadSE); g_enemy_deadSE = -1; }
}



//==============================================================================
// 初期化
//==============================================================================
void Enemy::Initialize(const XMFLOAT3& position)
{
    m_Position = position;
    m_Velocity = { 0,0,0 };
    m_Front = { 0,0,1 };
    m_Destination = MapPatrolAI_GetReachableDestination(m_Position); // 初期目的地設定(視界内の目的地のみ設定に変更)
    m_IsAlive = true;
    m_IsGround = false;
    m_WasChasing = false;
    //＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
    // HP設定
    //＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
    SetHP(300, 300);

    m_pModel = ModelLoad("resource/Models/enemy.fbx", ENEMY_SIZE);

    Enemy_LoadSE();
}

//==============================================================================
// 終了処理
//==============================================================================
void Enemy::Finalize()
{
    ModelRelease(m_pModel);
    m_pModel = nullptr;
}

//==============================================================================
// 更新処理
//==============================================================================
void Enemy::Update(double elapsed_time)
{
    if (!m_IsAlive) return;
    if (elapsed_time > 1.0 / 30.0)
        elapsed_time = 1.0 / 30.0;

    float dt = static_cast<float>(elapsed_time);

    XMVECTOR pos = XMLoadFloat3(&m_Position);
    XMVECTOR vel = XMLoadFloat3(&m_Velocity);

    // 　AI処理（追跡 + 巡回）
    EnemyAI_Update(
        &m_Position,
        &m_Velocity,
        &m_Front,
        &m_Destination,
        &m_WasChasing,
        dt,
        CHASE_SPD,   // 通常エネミーの追跡速度
        PATROL_SPD,  // 通常エネミーの巡回速度
        SIGHT_DIST); // 視野距離

    // AI処理後の速度を再ロード
    vel = XMLoadFloat3(&m_Velocity);

    // 重力加算
    vel += XMVectorSet(0, -9.8f * GRAVITY_MUL * dt, 0, 0);

    // 速度制限
    vel = ClampXZSpeed(vel, MAX_SPEED);

    // 摩擦
    vel += -vel * (FRICTION * dt);

    // サブステップ移動
    MoveWithSubSteps(&pos, &vel, dt);

    // 床判定
    ResolveFloorCollision(&pos, &vel);

    // 　プレイヤー衝突（ダメージ＋ノックバック）
    ResolvePlayerCollision(&pos, &vel);

    // 位置・速度を保存
    XMStoreFloat3(&m_Position, pos);
    XMStoreFloat3(&m_Velocity, vel);

    // 弾ヒット処理
    ResolveBulletHits();

    // 死亡判定
    if (IsDead()) m_IsAlive = false;
}

//==============================================================================
// 描画処理
//==============================================================================
void Enemy::Draw()
{
    if (!m_IsAlive) return;
    if (!m_pModel) return;

    Light_SetSpecularWorld(
        Player_Camera_GetPosition(),
        4.0f,
        { 0.3f,0.25f,0.2f,1.0f }
    );

    float angle =
        -atan2f(m_Front.z, m_Front.x)
        + XMConvertToRadians(0.0f);

    XMMATRIX rot =
        XMMatrixRotationX(XMConvertToRadians(00.0f)) *
        XMMatrixRotationY(angle) *
        XMMatrixRotationY(XMConvertToRadians(-90.0f));

    XMMATRIX trans =
        XMMatrixTranslation(
            m_Position.x,
            m_Position.y + ENEMY_HEIGHT * 1.0f,
            m_Position.z);

    ModelDraw(m_pModel, rot * trans);
}

//==============================================================================
// OBB取得
//==============================================================================
OBB Enemy::GetOBB() const
{
    XMFLOAT3 halfExtents = { ENEMY_HALF_WIDTH_X, ENEMY_HEIGHT * 0.5f, ENEMY_HALF_WIDTH_Z };

    XMFLOAT3 center = {
        m_Position.x,
        m_Position.y + ENEMY_HEIGHT * 0.5f,
        m_Position.z
    };

    return OBB::CreateFromFront(center, halfExtents, m_Front);
}

//==============================================================================
// AABB取得（デバッグ用・旧コード互換用）
//==============================================================================
AABB Enemy::GetAABB() const
{
    return {
        {
            m_Position.x - ENEMY_HALF_WIDTH_X,
            m_Position.y,
            m_Position.z - ENEMY_HALF_WIDTH_Z
        },
        {
            m_Position.x + ENEMY_HALF_WIDTH_X,
            m_Position.y + ENEMY_HEIGHT,
            m_Position.z + ENEMY_HALF_WIDTH_Z
        }
    };
}

//==============================================================================
// 位置取得
//==============================================================================
const XMFLOAT3& Enemy::GetPosition() const
{
    return m_Position;
}

//==============================================================================
// 速度ポインタ取得（ノックバック用）
//==============================================================================
DirectX::XMFLOAT3* Enemy::GetVelocityPtr()
{
    return &m_Velocity;
}

//==============================================================================
// HP設定
//==============================================================================
void Enemy::SetHP(int hp, int maxHp)
{
    m_MaxHp = std::max(1, maxHp);
    m_Hp = std::clamp(hp, 0, m_MaxHp);
}

//==============================================================================
// ダメージ処理
//==============================================================================
void Enemy::Damage(int value)
{
    m_Hp -= std::max(1, value);
    if (m_Hp < 0) m_Hp = 0;
}

//==============================================================================
// HP取得
//==============================================================================
int Enemy::GetHP() const { return m_Hp; }

//==============================================================================
// 最大HP取得
//==============================================================================
int Enemy::GetMaxHP() const { return m_MaxHp; }

//==============================================================================
// 死亡判定
//==============================================================================
bool Enemy::IsDead() const { return m_Hp <= 0; }

//==============================================================================
// 内部ヘルパー関数
//==============================================================================

//==============================================================================
// 任意の位置ベクトルからエネミー用 OBB を作成
//==============================================================================
OBB Enemy::ConvertPositionToOBB(const XMVECTOR& pos) const
{
    XMFLOAT3 position;
    XMStoreFloat3(&position, pos);

    XMFLOAT3 halfExtents = { ENEMY_HALF_WIDTH_X, ENEMY_HEIGHT * 0.5f, ENEMY_HALF_WIDTH_Z };

    XMFLOAT3 center = {
        position.x,
        position.y + ENEMY_HEIGHT * 0.5f,
        position.z
    };

    return OBB::CreateFromFront(center, halfExtents, m_Front);
}

//==============================================================================
// XZ速度制限
//==============================================================================
XMVECTOR Enemy::ClampXZSpeed(XMVECTOR v, float maxSpeed) const
{
    float vx = XMVectorGetX(v);
    float vz = XMVectorGetZ(v);
    float lenSq = vx * vx + vz * vz;
    if (lenSq <= maxSpeed * maxSpeed) return v;

    float inv = 1.0f / sqrtf(lenSq);
    return XMVectorSet(
        vx * inv * maxSpeed,
        XMVectorGetY(v),
        vz * inv * maxSpeed,
        0);
}

//==============================================================================
// 壁衝突解決（OBB ↔ AABB）
//==============================================================================
void Enemy::ResolveWallCollisionAtPosition(XMVECTOR* ioPos, XMVECTOR* ioVel, XMFLOAT3* ioDest)
{
    constexpr float TELEPORT_THRESHOLD = 0.3f;

    for (int i = 0; i < Map_GetObjectsCount(); ++i)
    {
        const MapObject* mo = Map_GetObject(i);
        if (!mo) continue;
        if (mo->KindId != 2) continue;

        OBB enemyOBB = ConvertPositionToOBB(*ioPos);
        Hit hit = Collision_IsHitOBB_AABB(enemyOBB, mo->Aabb);
        if (!hit.isHit) continue;

        const AABB& a = mo->Aabb;
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, *ioPos);

        const float overlapX = (ENEMY_HALF_WIDTH_X + (a.max.x - a.min.x) * 0.5f)
            - fabsf(pos.x - (a.min.x + a.max.x) * 0.5f);
        const float overlapZ = (ENEMY_HALF_WIDTH_Z + (a.max.z - a.min.z) * 0.5f)
            - fabsf(pos.z - (a.min.z + a.max.z) * 0.5f);

        const float minOverlap = std::min(overlapX, overlapZ);

        // めり込み量が閾値以上ならテレポート
        if (minOverlap >= TELEPORT_THRESHOLD)
        {
            XMFLOAT3 safePos = MapPatrolAI_GetNearbyDestination(pos, 3.0f);
            *ioPos = XMLoadFloat3(&safePos);
            *ioVel = XMVectorSet(0.0f, XMVectorGetY(*ioVel), 0.0f, 0.0f);
            *ioDest = MapPatrolAI_GetNearbyDestination(safePos, 3.0f);
            break;
        }

        // 通常の押し戻し
        if (overlapX < overlapZ)
        {
            const float sign = (pos.x < (a.min.x + a.max.x) * 0.5f) ? -1.0f : 1.0f;
            *ioPos = XMVectorSetX(*ioPos, pos.x + sign * overlapX);
            *ioVel = XMVectorSetX(*ioVel, 0.0f);
        }
        else
        {
            const float sign = (pos.z < (a.min.z + a.max.z) * 0.5f) ? -1.0f : 1.0f;
            *ioPos = XMVectorSetZ(*ioPos, pos.z + sign * overlapZ);
            *ioVel = XMVectorSetZ(*ioVel, 0.0f);
        }

        // 押し戻し後の位置から3マス以内の目的地を設定する
        XMFLOAT3 newPos;
        XMStoreFloat3(&newPos, *ioPos);
        *ioDest = MapPatrolAI_GetNearbyDestination(newPos, 3.0f);
        break;
    }
}

//==============================================================================
// サブステップ移動（トンネリング対策）
//==============================================================================
void Enemy::MoveWithSubSteps(XMVECTOR* ioPos, XMVECTOR* ioVel, float dt)
{
    XMVECTOR delta = (*ioVel) * dt;
    float len = sqrtf(
        powf(XMVectorGetX(delta), 2.0f) +
        powf(XMVectorGetZ(delta), 2.0f)
    );

    int steps = static_cast<int>(ceilf(len / SUBSTEP_MAX_STEP));
    if (steps < 1) steps = 1;
    if (steps > SUBSTEP_MAX_COUNT) steps = SUBSTEP_MAX_COUNT;

    float stepDt = dt / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i)
    {
        *ioPos += (*ioVel) * stepDt;
        ResolveWallCollisionAtPosition(ioPos, ioVel, &m_Destination);
    }
}

//==============================================================================
// 床衝突解決（OBB ↔ AABB）
//==============================================================================
void Enemy::ResolveFloorCollision(XMVECTOR* ioPos, XMVECTOR* ioVel)
{
    OBB enemyOBB = ConvertPositionToOBB(*ioPos);
    float supportY = 0.0f;
    bool foundFloor = false;

    for (int i = 0; i < Map_GetObjectsCount(); ++i)
    {
        const MapObject* mo = Map_GetObject(i);
        if (!mo) continue;
        if (mo->KindId != 1) continue; // KIND_FLOOR

        const AABB& floor = mo->Aabb;

        // XZ重なり判定（簡易）
        bool overlapX = (enemyOBB.center.x - ENEMY_HALF_WIDTH_X <= floor.max.x &&
            enemyOBB.center.x + ENEMY_HALF_WIDTH_X >= floor.min.x);
        bool overlapZ = (enemyOBB.center.z - ENEMY_HALF_WIDTH_Z <= floor.max.z &&
            enemyOBB.center.z + ENEMY_HALF_WIDTH_Z >= floor.min.z);

        if (!overlapX || !overlapZ) continue;

        const float eps = 0.02f;
        float enemyBottom = XMVectorGetY(*ioPos);

        if (enemyBottom <= floor.max.y + eps)
        {
            if (!foundFloor || floor.max.y > supportY)
            {
                supportY = floor.max.y;
                foundFloor = true;
            }
        }
    }

    if (foundFloor)
    {
        float currentY = XMVectorGetY(*ioPos);
        if (currentY <= supportY)
        {
            *ioPos = XMVectorSetY(*ioPos, supportY);
            *ioVel = XMVectorSetY(*ioVel, 0.0f);
            m_IsGround = true;
        }
    }
    else
    {
        m_IsGround = false;
    }
}

//==============================================================================
// プレイヤー衝突処理（ダメージ＋ノックバック）
//==============================================================================
void Enemy::ResolvePlayerCollision(XMVECTOR* ioPos, XMVECTOR* ioVel)
{
    // プレイヤーが無効または無敵中はスキップ
    if (!Player_IsEnable() || Player_IsInvincible())
    {
        return;
    }

    OBB enemyOBB  = ConvertPositionToOBB(*ioPos);
    OBB playerOBB = Player_GetOBB();

    // OBB同士の衝突判定
    if (!Collision_IsOverlapOBB(enemyOBB, playerOBB))
    {
        return;
    }

    // 　ダメージ処理
    constexpr int ENEMY_DAMAGE = 10;
    Player_TakeDamage(ENEMY_DAMAGE);
    // 射撃音再生（単発）
    PlayAudio(g_enemy_hitSE, false);

    // 　ノックバック処理（プレイヤーを押し出す）
    XMFLOAT3* playerVelPtr = Player_GetVelocityPtr();
    XMFLOAT3 playerPosF = Player_GetPosition();

    // プレイヤーからエネミーへのベクトル（逆向き = 押し出し方向）
    XMVECTOR toEnemy = XMLoadFloat3(&m_Position) - XMLoadFloat3(&playerPosF);
    toEnemy = XMVectorSetY(toEnemy, 0.0f); // Y成分無視（水平方向のみ）

    // 正規化してノックバック速度を加算
    if (XMVectorGetX(XMVector3LengthSq(toEnemy)) > 0.0001f)
    {
        constexpr float KNOCKBACK_STRENGTH = 15.0f; // ノックバック強度（5.0f = 弱い / 15.0f = 強い / 25.0f = 超強い）
        XMVECTOR knockback = XMVector3Normalize(toEnemy) * -KNOCKBACK_STRENGTH;
        XMVECTOR currentVel = XMLoadFloat3(playerVelPtr);
        XMStoreFloat3(playerVelPtr, currentVel + knockback);
    }
}

//==============================================================================
// 弾ヒット処理（OBB ↔ OBB）
//==============================================================================
void Enemy::ResolveBulletHits()
{
    if (!m_IsAlive) return;

    for (int i = 0; i < Bullet_GetCount(); ++i)
    {
        OBB bulletOBB = Bullet_GetOBB(i);
        OBB enemyOBB  = GetOBB();

        if (!Collision_IsOverlapOBB(bulletOBB, enemyOBB))
            continue;

        //ヒットSE
        PlayAudio(g_enemy_hitSE, false);

        // ダメージ量を取得
        const int bulletDamage = Bullet_GetDamage(i);

        // ビーム弾は貫通するため削除しない
        if (!Bullet_IsBeam(i))
        {
            Bullet_Destroy(i);
            --i;
        }

        Damage(bulletDamage);

        if (IsDead())
        {
            //死亡SE
            PlayAudio(g_enemy_deadSE, false);
            Score_Addscore(1000);//1000点追加

            ItemManager_SpawnRandom(m_Position);//アイテムをランダムでスポーン
            break;
        }
    }
}
