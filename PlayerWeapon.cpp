/*==============================================================================

   プレイヤー武器システム [PlayerWeapon.cpp]
                                                         Author : 51106
                                                         Date   : 2026/04/01
--------------------------------------------------------------------------------

==============================================================================*/
#include "PlayerWeapon.h"
#include "bullet.h"
#include "Audio.h"
#include "game.h"     // Game_GetLockOnWorldPos
#include <DirectXMath.h>
#include <cstdlib>   // rand()
#include <cmath>
#include <algorithm> // std::clamp

using namespace DirectX;

//==============================================================================
// WeaponNormal（通常弾）
//==============================================================================

void WeaponNormal::Initialize()
{
    m_cooldown = 0.0;
    m_shootSE  = LoadAudioWithVolume("resource/sound/machine_gun.wav", 0.8f);
}

void WeaponNormal::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponNormal::Update(double dt)
{
    if (m_cooldown > 0.0) m_cooldown -= dt;
}

bool WeaponNormal::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    XMFLOAT3 vel;
    XMStoreFloat3(&vel,
        XMVector3Normalize(XMLoadFloat3(&aimDir)) * BULLET_SPEED);

    Bullet_Create(muzzlePos, vel, finalDamage);

    if (m_shootSE >= 0) PlayAudio(m_shootSE, false);

    m_cooldown = FIRE_INTERVAL;
    return true;
}


//==============================================================================
// WeaponTripleGun（トリプルマシンガン：横3列平行発射）
//==============================================================================

void WeaponTripleGun::Initialize()
{
    m_cooldown = 0.0;
    m_shootSE  = LoadAudioWithVolume("resource/sound/machine_gun.wav", 0.8f);
}

void WeaponTripleGun::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponTripleGun::Update(double dt)
{
    if (m_cooldown > 0.0) m_cooldown -= dt;
}

bool WeaponTripleGun::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    XMVECTOR aimV = XMVector3Normalize(XMLoadFloat3(&aimDir));

    // 横方向（右）ベクトル：aim が真上/真下のときは別軸を基準にする
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right;
    if (fabsf(XMVectorGetX(XMVector3Dot(aimV, up))) > 0.99f)
        right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    else
        right = XMVector3Normalize(XMVector3Cross(up, aimV));

    // 3列とも同じ方向（平行）。速度ベクトルは共通
    XMFLOAT3 vel;
    XMStoreFloat3(&vel, aimV * BULLET_SPEED);

    // マズルを横にずらして中央・右・左の3列を平行発射（拡散なし）
    const float offsets[BARREL_COUNT] = { 0.0f, BARREL_SPACING, -BARREL_SPACING };
    XMVECTOR muzzleV = XMLoadFloat3(&muzzlePos);
    for (int i = 0; i < BARREL_COUNT; ++i)
    {
        XMFLOAT3 mp;
        XMStoreFloat3(&mp, muzzleV + right * offsets[i]);
        Bullet_Create(mp, vel, finalDamage);
    }

    if (m_shootSE >= 0) PlayAudio(m_shootSE, false);

    m_cooldown = FIRE_INTERVAL;
    return true;
}


//==============================================================================
// WeaponMissile（ミサイル）
//==============================================================================

void WeaponMissile::Initialize()
{
    m_cooldown = 0.0;
    m_shootSE  = LoadAudioWithVolume("resource/sound/rocket_launcher.wav", 0.8f);
}

void WeaponMissile::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponMissile::Update(double dt)
{
    if (m_cooldown > 0.0) m_cooldown -= dt;
}

bool WeaponMissile::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    XMFLOAT3 vel;
    XMStoreFloat3(&vel,
        XMVector3Normalize(XMLoadFloat3(&aimDir)) * BULLET_SPEED);

    Bullet_CreateMissile(muzzlePos, vel, finalDamage, EXPLOSION_RADIUS);

    if (m_shootSE >= 0) PlayAudio(m_shootSE, false);

    m_cooldown = FIRE_INTERVAL;
    return true;
}


//==============================================================================
// WeaponMultiMissile（マルチミサイル）
//==============================================================================

void WeaponMultiMissile::Initialize()
{
    m_cooldown = 0.0;
    m_shootSE  = LoadAudioWithVolume("resource/sound/rocket_launcher.wav", 0.8f); // ミサイルと同じ発射音
}

void WeaponMultiMissile::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponMultiMissile::Update(double dt)
{
    if (m_cooldown > 0.0) m_cooldown -= dt;
}

bool WeaponMultiMissile::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    const XMVECTOR vMuzzle = XMLoadFloat3(&muzzlePos);
    const XMVECTOR vAim    = XMVector3Normalize(XMLoadFloat3(&aimDir));

    // 標的位置：ロックオン優先、なければ照準方向の前方 TARGET_FORWARD_DIST
    XMVECTOR vTarget;
    XMFLOAT3 lockPos;
    if (Game_GetLockOnWorldPos(&lockPos))
        vTarget = XMLoadFloat3(&lockPos);
    else
        vTarget = vMuzzle + vAim * TARGET_FORWARD_DIST;

    // 標的への方向・距離
    XMVECTOR toTarget = vTarget - vMuzzle;
    const float dist  = sqrtf(XMVectorGetX(XMVector3LengthSq(toTarget)));
    XMVECTOR dir      = (dist > 0.0001f) ? XMVector3Normalize(toTarget) : vAim;

    // 直交基底（横 right / 上 up）
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR rightB  = XMVector3Cross(worldUp, dir);
    if (XMVectorGetX(XMVector3LengthSq(rightB)) < 0.0001f)
        rightB = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);   // 真上/真下を向いている場合の保険
    rightB = XMVector3Normalize(rightB);
    XMVECTOR upB = XMVector3Normalize(XMVector3Cross(dir, rightB));

    const XMVECTOR mid = (vMuzzle + vTarget) * 0.5f;

    // 曲線を飛び切る時間（距離ベースでクランプ）
    const float duration = std::clamp(dist / BULLET_SPEED, 0.35f, 1.2f);

    // MISSILE_COUNT 本を横に均等配置し、弧を描いて拡散 → 標的へ収束
    for (int i = 0; i < MISSILE_COUNT; ++i)
    {
        const float lat = (MISSILE_COUNT > 1)
            ? (static_cast<float>(i) / (MISSILE_COUNT - 1)) * 2.0f - 1.0f  // -1..+1
            : 0.0f;

        const float lateral = lat * SPREAD_WIDTH;                 // 横方向の膨らみ
        const float arc     = ARC_HEIGHT * (1.0f - lat * lat);    // 中央ほど高く弧を描く

        // 制御点：中点から横＋上へオフセット
        const XMVECTOR p1 = mid + rightB * lateral + upB * (arc + ARC_HEIGHT * 0.5f);

        XMFLOAT3 f0, f1, f2;
        XMStoreFloat3(&f0, vMuzzle);
        XMStoreFloat3(&f1, p1);
        XMStoreFloat3(&f2, vTarget);

        Bullet_CreateMissileBezier(f0, f1, f2, duration, BULLET_SPEED, finalDamage, EXPLOSION_RADIUS);
    }

    if (m_shootSE >= 0) PlayAudio(m_shootSE, false);

    m_cooldown = FIRE_INTERVAL;
    return true;
}


//==============================================================================
// WeaponBeam（ビーム）
//==============================================================================

void WeaponBeam::Initialize()
{
    m_cooldown   = 0.0;
    m_seCooldown = 0.0;
    m_energy     = ENERGY_MAX;
    m_shootSE    = LoadAudioWithVolume("resource/sound/beam_shoot.wav", 0.5f);
}

void WeaponBeam::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponBeam::Update(double dt)
{
    if (m_cooldown   > 0.0) m_cooldown   -= dt;
    if (m_seCooldown > 0.0) m_seCooldown -= dt;

    // エネルギー自動回復（1分で全回復ペース）。
    // 飛行時は消費(150/秒)の方が大きいので飛びっぱなしにはならない。
    if (m_energy < ENERGY_MAX)
    {
        m_energy += ENERGY_REGEN * static_cast<float>(dt);
        if (m_energy > ENERGY_MAX) m_energy = ENERGY_MAX;
    }
}

bool WeaponBeam::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0 || m_energy < ENERGY_COST) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    // Bullet_CreateBeam は内部で速度を正規化するので方向だけ渡す
    XMFLOAT3 dir;
    XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&aimDir)));
    Bullet_CreateBeam(muzzlePos, dir, finalDamage);

    m_energy -= ENERGY_COST;
    if (m_energy < 0.0f) m_energy = 0.0f;

    // SE は連射でも一定間隔以上空けてから鳴らす
    if (m_seCooldown <= 0.0 && m_shootSE >= 0)
    {
        PlayAudio(m_shootSE, false);
        m_seCooldown = SE_INTERVAL;
    }

    m_cooldown = FIRE_INTERVAL;
    return true;
}

void WeaponBeam::AddEnergy(float amount)
{
    m_energy += amount;
    if (m_energy > ENERGY_MAX) m_energy = ENERGY_MAX;
}


//==============================================================================
// WeaponShotgun（ショットガン）
//==============================================================================

void WeaponShotgun::Initialize()
{
    m_cooldown = 0.0;
    m_shootSE  = LoadAudioWithVolume("resource/sound/shotgun.wav", 0.2f);
}

void WeaponShotgun::Finalize()
{
    UnloadAudio(m_shootSE);
    m_shootSE = -1;
}

void WeaponShotgun::Update(double dt)
{
    if (m_cooldown > 0.0) m_cooldown -= dt;
}

bool WeaponShotgun::TryFire(
    const XMFLOAT3& muzzlePos,
    const XMFLOAT3& aimDir,
    float           damageMult)
{
    if (m_cooldown > 0.0) return false;

    const int finalDamage = static_cast<int>(BASE_DAMAGE * damageMult);

    XMVECTOR aimV = XMVector3Normalize(XMLoadFloat3(&aimDir));
    XMVECTOR up   = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // aimDir が真上／真下の場合は別軸を基準に
    XMVECTOR right;
    float dotUp = fabsf(XMVectorGetX(XMVector3Dot(aimV, up)));
    if (dotUp > 0.99f)
        right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    else
        right = XMVector3Normalize(XMVector3Cross(up, aimV));

    XMVECTOR pelletUp = XMVector3Normalize(XMVector3Cross(aimV, right));

    const float spreadRad = XMConvertToRadians(SPREAD_DEG);

    for (int p = 0; p < PELLET_COUNT; ++p)
    {
        // 各ペレットにランダムな上下左右のブレを付ける
        float yawOff   = ((rand() % 2001) - 1000) / 1000.0f * spreadRad;
        float pitchOff = ((rand() % 2001) - 1000) / 1000.0f * spreadRad;

        XMVECTOR spreadDir = XMVector3TransformNormal(
            aimV,
            XMMatrixRotationAxis(pelletUp, yawOff) *
            XMMatrixRotationAxis(right,    pitchOff));

        XMFLOAT3 pelletVel;
        XMStoreFloat3(&pelletVel,
            XMVector3Normalize(spreadDir) * BULLET_SPEED);

        Bullet_Create(muzzlePos, pelletVel, finalDamage);
    }

    if (m_shootSE >= 0) PlayAudio(m_shootSE, false);

    m_cooldown = FIRE_INTERVAL;
    return true;
}
