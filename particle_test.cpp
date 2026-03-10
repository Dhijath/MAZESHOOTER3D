/*==============================================================================
   パーティクルテスト [particle_test.cpp]
                                                         Author : 51106
                                                         Date   : 2026/02/06
--------------------------------------------------------------------------------
   NormalParticle と NormalEmitter の実装　動かないので現在は動くように補修
==============================================================================*/
#include "particle_test.h"
#include "billboard.h"
#include "direct3d.h"

//==============================================================================
// NormalParticle クラスの実装
//==============================================================================

NormalParticle::NormalParticle(
    const DirectX::XMVECTOR& position,
    const DirectX::XMVECTOR& velocity,
    double Life_Time,
    double Spawn_Time,
    int texture_id,
    float scale)
    : Particle(position, velocity, Life_Time, Spawn_Time)
    , m_texture_id(texture_id)
    , m_scale(scale)
{
}

NormalParticle::~NormalParticle()
{
}

void NormalParticle::Update(double elapsed_time)
{
    // スケールをフェードアウト
    float ratio = std::min(static_cast<float>(GetAccumulatedTime() / GetLifeTime()), 1.0f);
    m_scale = 1.0f - ratio;

    // 重力を加える
    Add_Velocity(DirectX::XMVectorScale(
        DirectX::XMVectorSet(0.0f, -9.8f, 0.0f, 0.0f),
        static_cast<float>(elapsed_time)
    ));

    // 基底クラスで位置更新
    Particle::Update(elapsed_time);
}

void NormalParticle::Draw() const
{
    DirectX::XMFLOAT3 position{};
    DirectX::XMStoreFloat3(&position, GetPosition());

    float ratio = static_cast<float>(GetAccumulatedTime() / GetLifeTime());
    if (ratio > 1.0f) ratio = 1.0f;

    float fadeScale = m_scale * (1.0f - ratio);

    Billboard_Draw(
        m_texture_id,
        position,
        { fadeScale, fadeScale },
        DirectX::XMFLOAT4{ 2.0f, 3.0f, 5.0f, 1.0f },  // 　 青白く光る
        DirectX::XMFLOAT4{ 0.0f, 0.0f, 80.0f, 80.0f });

}


//==============================================================================
// NormalEmitter クラスの実装
//==============================================================================

NormalEmitter::NormalEmitter(
    const DirectX::XMVECTOR& position,
    double particles_per_second,
    bool is_emmit)
    : Emitter(position, particles_per_second, is_emmit)
    , m_mt(std::random_device()())
    , m_dir_dist(-DirectX::XM_PI, DirectX::XM_PI)
    , m_length_dist(-DirectX::XM_PI, DirectX::XM_PI)
    , m_speed_dist(2.0f, 5.0f)
    , m_lifetime_dist(0.5f, 1.0f)
{
}

NormalEmitter::~NormalEmitter()
{
}

Particle* NormalEmitter::createParticle()
{
    // ランダムな角度
    float angle = m_dir_dist(m_mt);

    // Y軸方向の角度（80度～100度）
    std::uniform_real_distribution<float> dist{
        DirectX::XMConvertToRadians(80.0f),
        DirectX::XMConvertToRadians(100.0f)
    };
    float y = dist(m_mt);

    // 方向ベクトルを計算
    DirectX::XMVECTOR direction{
        cosf(angle),
        sinf(DirectX::XMConvertToRadians(90.0f)),
        sinf(angle),
        0.0f
    };

    // 正規化
    direction = DirectX::XMVector3Normalize(direction);

    // 速度ベクトル = 方向 × スピード
    DirectX::XMVECTOR velocity = DirectX::XMVectorScale(direction, m_speed_dist(m_mt));

    // NormalParticle を生成
    return new NormalParticle(
        GetPosition(),              // エミッターの位置
        velocity,                   // 速度
        m_lifetime_dist(m_mt),      // 寿命
        0.0,                        // スポーン時間
        m_particle_texture_id,      // テクスチャID
        1.0f                        // スケール
    );
}

void NormalEmitter::Update(double elapsed_time)
{
    // 基底クラスの更新
    Emitter::Update(elapsed_time);
}

void NormalEmitter::Draw()
{
    Direct3D_SetBlendStateAdditive(true);
    Direct3D_SetDepthStencilStateDepthWriteDisable(false);  // 　 false = 深度書き込み無効

    const auto& particles = GetParticles();
    for (const auto* particle : particles) {
        if (particle && !particle->IsDestroy()) {
            const NormalParticle* normal_particle = dynamic_cast<const NormalParticle*>(particle);
            if (normal_particle) {
                normal_particle->Draw();
            }
        }
    }

    Direct3D_SetBlendStateAdditive(false);
    Direct3D_SetDepthStencilStateDepthWriteDisable(true);  // 　 true = 深度書き込み有効に戻す
}