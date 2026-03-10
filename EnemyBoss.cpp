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

using namespace DirectX;

//==============================================================================
// 初期化
//
// ■役割
// ・Enemy::Initialize で基本状態（位置・AI状態・SE・通常モデル）をセットアップ
// ・その後、HP・モデル・SEをボス用に上書きする
//==============================================================================
void EnemyBoss::Initialize(const XMFLOAT3& position)
{
    // 基底クラスの初期化（位置・AI状態・SE・通常モデルをロード）
    Enemy::Initialize(position);

    // HP をボス用に上書き
    SetHP(HP, HP);

    // モデルを大きいサイズで差し替え
    ModelRelease(m_pModel);
    m_pModel = ModelLoad("resource/Models/enemy_tank.fbx", BOSS_SIZE);

    // ショットガンSEをロード
    m_shootSE    = LoadAudioWithVolume("resource/sound/shotgun.wav", 0.5f);
    m_shootTimer = 0.0f;
}

//==============================================================================
// 終了処理
//==============================================================================
void EnemyBoss::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;

    Enemy::Finalize();
}

//==============================================================================
// 更新処理
//
// ■役割
// ・基底クラスの Update（AI移動・重力・衝突・弾ヒット・死亡判定）を実行
// ・生存中のみ射撃タイマーを進め、インターバル到達＆視線が通れば Shoot() を呼ぶ
//==============================================================================
void EnemyBoss::Update(double elapsed_time)
{
    if (!m_IsAlive) return;

    // 基底クラスのUpdate（AI・移動・衝突・死亡判定）
    Enemy::Update(elapsed_time);

    // 死亡後は射撃しない
    if (!m_IsAlive) return;

    float dt = static_cast<float>(elapsed_time > 1.0 / 30.0 ? 1.0 / 30.0 : elapsed_time);

    m_shootTimer += dt;
    if (m_shootTimer >= SHOOT_INTERVAL)
    {
        m_shootTimer = 0.0f;

        // 視線が通っていれば射撃
        if (MapPatrolAI_HasLineOfSight(m_Position, Player_GetPosition()))
        {
            Shoot();
        }
    }
}

//==============================================================================
// 射撃処理
//
// ■役割
// ・ボス正面に直交する right ベクトルを求め、左右バレル位置を算出
// ・各バレルからプレイヤーの胴体に向けて弾を1発ずつ発射する
// ・発射と同時にショットガンSEを再生する
//==============================================================================
void EnemyBoss::Shoot()
{
    XMFLOAT3 playerPos = Player_GetPosition();

    // front に直交する右方向を計算（プレイヤーの playerRight と同じ手順）
    XMVECTOR frontV = XMVectorSet(m_Front.x, 0.0f, m_Front.z, 0.0f);
    XMVECTOR upV    = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR rightV = XMVector3Normalize(XMVector3Cross(upV, frontV));

    XMFLOAT3 right;
    XMStoreFloat3(&right, rightV);

    // バレルの高さ：ボスボディ底面（プレイヤーの bodyAABB.min.y 相当）
    float barrelY = m_Position.y;

    // 左バレル位置（右方向＋前方オフセット、プレイヤーの barrelOriginPos と同じ考え方）
    XMFLOAT3 leftBarrel = {
        m_Position.x + right.x * BARREL_OFFSET + m_Front.x * BARREL_FORWARD,
        barrelY,
        m_Position.z + right.z * BARREL_OFFSET + m_Front.z * BARREL_FORWARD
    };

    // 右バレル位置（右方向反転＋前方オフセット）
    XMFLOAT3 rightBarrel = {
        m_Position.x - right.x * BARREL_OFFSET + m_Front.x * BARREL_FORWARD,
        barrelY,
        m_Position.z - right.z * BARREL_OFFSET + m_Front.z * BARREL_FORWARD
    };

    // プレイヤーの胴体（中心）を狙う
    XMFLOAT3 target = {
        playerPos.x,
        playerPos.y + 0.3f,
        playerPos.z
    };

    // 左バレルから発射
    {
        XMVECTOR dir = XMLoadFloat3(&target) - XMLoadFloat3(&leftBarrel);
        XMFLOAT3 vel;
        XMStoreFloat3(&vel, XMVector3Normalize(dir));
        EnemyBullet_Create(leftBarrel, vel, SHOOT_DAMAGE, BOSS_BULLET_SPEED);
    }

    // 右バレルから発射
    {
        XMVECTOR dir = XMLoadFloat3(&target) - XMLoadFloat3(&rightBarrel);
        XMFLOAT3 vel;
        XMStoreFloat3(&vel, XMVector3Normalize(dir));
        EnemyBullet_Create(rightBarrel, vel, SHOOT_DAMAGE, BOSS_BULLET_SPEED);
    }

    // ショットガンSE再生
    PlayAudio(m_shootSE, false);
}
