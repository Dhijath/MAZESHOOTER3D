/*==============================================================================

   ボス登場演出 [BossIntro.cpp]
                                                         Author : 51106
                                                         Date   : 2026/03/13
--------------------------------------------------------------------------------
   ■フェーズ
     ORBIT  (3.0秒) : ボス位置を中心に半径 5m、270度旋回
                       高さ: sin 波で 1.0〜3.0m の間を変動
                       カメラは常にボスを注視
     RETURN (1.0秒) : 軌道終了位置からプレイヤーカメラ位置へ lerp
     DONE           : Player_SetEnable(true) で操作権を返す

   ■カメラ上書き
     Player_Camera_OverrideCinematic(eyePos, targetPos) を毎フレーム呼ぶ

==============================================================================*/
#include "BossIntro.h"
#include "Player_Camera.h"
#include "Player.h"
#include "game.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

//==============================================================================
// 内部状態
//==============================================================================
namespace
{
    enum class Phase { Idle, Orbit, Return, Done };

    static Phase    g_Phase     = Phase::Idle;
    static double   g_Timer     = 0.0;     // フェーズ内経過時間

    // ボスのスポーン座標
    static XMFLOAT3 g_BossPos   = {};

    // ORBIT フェーズ終了時のカメラ座標（RETURN 開始点）
    static XMFLOAT3 g_OrbitEndEye = {};

    // 演出開始時のプレイヤーカメラ座標（RETURN の終点）
    static XMFLOAT3 g_PreIntroEye = {};

    // --- 定数 ---
    static constexpr double ORBIT_DURATION  = 3.0;   // 旋回時間（秒）
    static constexpr double RETURN_DURATION = 1.0;   // 戻り時間（秒）
    static constexpr float  ORBIT_RADIUS    = 5.0f;  // 旋回半径（m）
    static constexpr float  ORBIT_ANGLE_RAD = XM_PI * 1.5f; // 270度（rad）
    static constexpr float  HEIGHT_MIN      = 1.0f;  // 最低高さ（m）
    static constexpr float  HEIGHT_MAX      = 3.0f;  // 最高高さ（m）

    //----------------------------------------------------------
    // ORBIT 進行 t=0〜1 のカメラ eye 座標を計算
    //----------------------------------------------------------
    static XMFLOAT3 CalcOrbitEye(float t)
    {
        const float angle  = ORBIT_ANGLE_RAD * t;   // 時計回り
        const float ex     = g_BossPos.x + ORBIT_RADIUS * cosf(angle);
        const float ez     = g_BossPos.z + ORBIT_RADIUS * sinf(angle);
        const float sinH   = sinf(t * XM_PI);         // 0→1→0 の滑らか山型
        const float height = HEIGHT_MIN + (HEIGHT_MAX - HEIGHT_MIN) * sinH;
        const float ey     = g_BossPos.y + height;
        return XMFLOAT3{ ex, ey, ez };
    }
}

//==============================================================================
// 演出開始
//==============================================================================
void BossIntro_Start(const XMFLOAT3& bossSpawnPos)
{
    g_BossPos     = bossSpawnPos;
    g_Phase       = Phase::Orbit;
    g_Timer       = 0.0;
    g_PreIntroEye = Player_Camera_GetPosition(); // 演出前のカメラ位置を保存

    // プレイヤー入力を無効化
    Player_SetEnable(false);

    // ボスをプレイヤー方向（演出開始前カメラ位置）に向ける
    // ※ BossIntro 中は Game_Update がスキップされ EnemyBoss::Update が呼ばれないため
    //   m_Front がゼロのままになり壁を向いて見える問題を防ぐ
    {
        float dx = g_PreIntroEye.x - g_BossPos.x;
        float dz = g_PreIntroEye.z - g_BossPos.z;
        float len = sqrtf(dx * dx + dz * dz);
        if (len > 0.001f)
            Game_SetBossLookDir({ dx / len, 0.0f, dz / len });
    }
}

//==============================================================================
// 更新（演出中のみ呼ぶ）
//==============================================================================
void BossIntro_Update(double dt)
{
    if (g_Phase == Phase::Idle || g_Phase == Phase::Done) return;

    g_Timer += dt;

    if (g_Phase == Phase::Orbit)
    {
        //------------------------------------------------------
        // ORBIT フェーズ：ボスを中心に旋回
        //------------------------------------------------------
        const float t   = static_cast<float>(g_Timer / ORBIT_DURATION);
        const float tc  = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
        const XMFLOAT3 eye = CalcOrbitEye(tc);

        Player_Camera_OverrideCinematic(eye, g_BossPos);

        if (g_Timer >= ORBIT_DURATION)
        {
            g_OrbitEndEye = eye;         // RETURN 開始点を保存
            g_Phase       = Phase::Return;
            g_Timer       = 0.0;
        }
    }
    else if (g_Phase == Phase::Return)
    {
        //------------------------------------------------------
        // RETURN フェーズ：軌道終点 → プレイヤーカメラ位置へ lerp
        //------------------------------------------------------
        const float t   = static_cast<float>(g_Timer / RETURN_DURATION);
        const float tc  = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

        // 滑らかな補間（ease-in/out）
        const float smooth = tc * tc * (3.0f - 2.0f * tc);

        XMVECTOR start  = XMLoadFloat3(&g_OrbitEndEye);
        XMVECTOR end    = XMLoadFloat3(&g_PreIntroEye);
        XMVECTOR eye    = XMVectorLerp(start, end, smooth);

        XMFLOAT3 eyeF;
        XMStoreFloat3(&eyeF, eye);

        // ターゲットはボスから徐々にプレイヤー視点の先方向へ
        Player_Camera_OverrideCinematic(eyeF, g_BossPos);

        if (g_Timer >= RETURN_DURATION)
        {
            g_Phase = Phase::Done;

            // プレイヤーカメラの保存済み View/Proj を復帰
            Player_Camera_ApplyMainViewProj();

            // プレイヤー入力を有効化
            Player_SetEnable(true);
        }
    }
}

//==============================================================================
// 演出中フラグ
//==============================================================================
bool BossIntro_IsPlaying()
{
    return (g_Phase == Phase::Orbit || g_Phase == Phase::Return);
}
