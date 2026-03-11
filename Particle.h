/*==============================================================================
   パーティクル制御 [Particle.h]
                                                         Author : 51106
                                                         Date   : 2026/02/04
--------------------------------------------------------------------------------
==============================================================================*/
#ifndef PARTICLE_H
#define PARTICLE_H

#include <DirectXMath.h>
#include <vector>

//==============================================================================
// Particle クラス
//==============================================================================
class Particle
{
private:
    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_velocity;

    double m_Life_Time{};
    double m_Spawn_Time{};
    double m_accumulated_time = 0;

public:
    //--------------------------------------------------------------------------
    // コンストラクタ
    //--------------------------------------------------------------------------
    Particle(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity,
        double Life_Time, double Spawn_Time)
        : m_position(position)
        , m_velocity(velocity)
        , m_Life_Time(Life_Time)
        , m_Spawn_Time(Spawn_Time)
    {
    }

    //--------------------------------------------------------------------------
    // デストラクタ
    //--------------------------------------------------------------------------
    virtual ~Particle() = default;

    //--------------------------------------------------------------------------
    // 破壊判定
    //--------------------------------------------------------------------------
    virtual bool IsDestroy() const {
        return m_accumulated_time >= m_Life_Time;
    }

    //--------------------------------------------------------------------------
    // 更新
    //--------------------------------------------------------------------------
    virtual void Update(double elapsed_time) {
        m_accumulated_time += elapsed_time;

        if (IsDestroy()) {
            return;
        }

        DirectX::XMVECTOR displacement = DirectX::XMVectorScale(m_velocity, static_cast<float>(elapsed_time));
        m_position = DirectX::XMVectorAdd(m_position, displacement);
    }

    //--------------------------------------------------------------------------
    // ゲッター
    //--------------------------------------------------------------------------
    const DirectX::XMVECTOR& GetPosition() const {
        return m_position;
    }

    const DirectX::XMVECTOR& GetVelocity() const {
        return m_velocity;
    }

    double GetLifeTime() const {
        return m_Life_Time;
    }

    double GetSpawnTime() const {
        return m_Spawn_Time;
    }

    double GetAccumulatedTime() const {
        return m_accumulated_time;
    }

    double GetRemainingTime() const {
        return m_Life_Time - m_accumulated_time;
    }

    double GetLifeRatio() const {
        if (m_Life_Time <= 0.0) return 0.0;
        return m_accumulated_time / m_Life_Time;
    }

protected:
    //--------------------------------------------------------------------------
    // セッター
    //--------------------------------------------------------------------------
    void SetPosition(const DirectX::XMVECTOR& position) {
        m_position = position;
    }

    void SetVelocity(const DirectX::XMVECTOR& velocity) {
        m_velocity = velocity;
    }

    void Add_Position(const DirectX::XMVECTOR& v) {
        m_position = DirectX::XMVectorAdd(m_position, v);
    }

    void Add_Velocity(const DirectX::XMVECTOR& v) {
        m_velocity = DirectX::XMVectorAdd(m_velocity, v);
    }

    void Destroy() {
        m_accumulated_time = m_Life_Time;
    }

    void ResetTime() {
        m_accumulated_time = 0.0;
    }
};

//==============================================================================
// Emitter クラス（噴出機）
//==============================================================================
class Emitter
{
private:
    DirectX::XMVECTOR m_position{};
    double m_particles_per_second{};
    double m_accumulated_time = 0.0;  // 累積時間
    bool m_is_emmit{};                // 放出フラグ

protected:
    std::vector<Particle*> m_particles;  // パーティクルリスト

    //--------------------------------------------------------------------------
    // パーティクル生成（純粋仮想関数）
    //--------------------------------------------------------------------------
    virtual Particle* createParticle() = 0;

public:
    //--------------------------------------------------------------------------
    // コンストラクタ
    //--------------------------------------------------------------------------
    Emitter(const DirectX::XMVECTOR& position, double particles_per_second, bool is_emmit = false)
        : m_position(position)
        , m_particles_per_second(particles_per_second)
        , m_is_emmit(is_emmit)
    {
    }

    //--------------------------------------------------------------------------
    // 仮想デストラクタ
    //--------------------------------------------------------------------------
    virtual ~Emitter() {
        // パーティクルの解放
        for (auto* particle : m_particles) {
            delete particle;
        }
        m_particles.clear();
    }

    //--------------------------------------------------------------------------
    // 更新
    //--------------------------------------------------------------------------
    virtual void Update(double elapsed_time) {
        // 放出が有効な場合、パーティクルを生成
        if (m_is_emmit) {
            m_accumulated_time += elapsed_time;

            // 1秒あたりの生成数に基づいて生成
            double emit_interval = 1.0 / m_particles_per_second;
            while (m_accumulated_time >= emit_interval) {
                Particle* particle = createParticle();
                if (particle) {
                    m_particles.push_back(particle);
                }
                m_accumulated_time -= emit_interval;
            }
        }

        // 全パーティクルを更新
        for (auto* particle : m_particles) {
            if (particle && !particle->IsDestroy()) {
                particle->Update(elapsed_time);
            }
        }

        // 死んだパーティクルを削除
        RemoveDeadParticles();
    }

    //--------------------------------------------------------------------------
    // 描画
    //--------------------------------------------------------------------------
    virtual void Draw() {
        // 派生クラスで実装
    }

    //--------------------------------------------------------------------------
    // パーティクル放出（手動）
    //--------------------------------------------------------------------------
    void Emmit(bool isEmmit) {
        m_is_emmit = isEmmit;
    }

    //--------------------------------------------------------------------------
    // 全パーティクルを即座に消去（ルーム遷移時など）
    //--------------------------------------------------------------------------
    void ClearAll() {
        for (auto* particle : m_particles) {
            delete particle;
        }
        m_particles.clear();
    }

    //--------------------------------------------------------------------------
    // 放出状態の取得
    //--------------------------------------------------------------------------
    bool IsEmmit() const {
        return m_is_emmit;
    }

    //--------------------------------------------------------------------------
    // ゲッター
    //--------------------------------------------------------------------------
    const DirectX::XMVECTOR& GetPosition() const {
        return m_position;
    }

    double GetParticlesPerSecond() const {
        return m_particles_per_second;
    }

    double GetAccumulatedTime() const {
        return m_accumulated_time;
    }

    const std::vector<Particle*>& GetParticles() const {
        return m_particles;
    }

    size_t GetParticleCount() const {
        return m_particles.size();
    }

    //--------------------------------------------------------------------------
    // セッター
    //--------------------------------------------------------------------------
    void SetPosition(const DirectX::XMVECTOR& position) {
        m_position = position;
    }

    void SetParticlesPerSecond(double particles_per_second) {
        m_particles_per_second = particles_per_second;
    }

protected:
    //--------------------------------------------------------------------------
    // 死んだパーティクルを削除
    //--------------------------------------------------------------------------
    void RemoveDeadParticles() {
        for (auto it = m_particles.begin(); it != m_particles.end(); ) {
            if (*it && (*it)->IsDestroy()) {
                delete* it;
                it = m_particles.erase(it);
            }
            else {
                ++it;
            }
        }
    }
};

#endif // PARTICLE_H