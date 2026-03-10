/*==============================================================================
   パーティクルテスト [particle_test.h]
                                                         Author : 51106
                                                         Date   : 2026/02/06
--------------------------------------------------------------------------------
   NormalParticle と NormalEmitter の定義
==============================================================================*/
#ifndef PARTICLE_TEST_H
#define PARTICLE_TEST_H

#include "Particle.h"
#include <random>

//==============================================================================
// NormalParticle クラス（ビルボード描画用）
//==============================================================================
class NormalParticle : public Particle
{
private:
    int m_texture_id;   // テクスチャID
    float m_scale;      // スケール

public:
    NormalParticle(
        const DirectX::XMVECTOR& position,
        const DirectX::XMVECTOR& velocity,
        double Life_Time,
        double Spawn_Time,
        int texture_id,
        float scale);

    virtual ~NormalParticle();

    virtual void Update(double elapsed_time) override;
    void Draw() const;

    int GetTextureId() const { return m_texture_id; }
    float GetScale() const { return m_scale; }
};

//==============================================================================
// NormalEmitter クラス（パーティクル生成器）
//==============================================================================
class NormalEmitter : public Emitter
{
private:
    std::mt19937 m_mt;
    std::uniform_real_distribution<float> m_dir_dist;
    std::uniform_real_distribution<float> m_length_dist;
    std::uniform_real_distribution<float> m_speed_dist;
    std::uniform_real_distribution<float> m_lifetime_dist;

    int m_particle_texture_id = 0;  // パーティクルのテクスチャID

protected:
    virtual Particle* createParticle() override;

public:
    NormalEmitter(
        const DirectX::XMVECTOR& position,
        double particles_per_second,
        bool is_emmit = false);

    virtual ~NormalEmitter();

    virtual void Update(double elapsed_time) override;
    virtual void Draw() override;

    // テクスチャID設定
    void SetParticleTextureId(int texId) {
        m_particle_texture_id = texId;
    }
};

#endif // PARTICLE_TEST_H