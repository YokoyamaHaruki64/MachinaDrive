#pragma once

#include "System/main.h"

#include "System/Component.h"
#include "Component/Primitive.h"
#include "Component/Transform.h"

#include "math/Vector3.h"
#include "math/Curve.h"
#include "Common/Color.h"

class ParticleEmitter : public Component
{
public:

    enum VelocityType : uint8_t
    {
        None = 0,       // 速度なし
        Directional = 1,// 単方向
        Cone = 2,       // コーン状
        SphereVel = 3,     // 球状ランダム
    };

    enum SpawnType : uint8_t
    {
        Point = 0,      // 点
        Circle = 1,     // 円
        Sphere = 2,     // 球
    };

    struct EmitterSettings
    {
        // 共通
        Vector3 offset = Vector3::Zero();   // パーティクルの生成位置のオフセット

        int maxParticles = 1000;            // パーティクルの最大数

        // バースト
        bool burst = true;              // true:ループせずワンショットで生成する
        int burstCount = 10;            // burstの合計生成量
        float burstTime = 0.1f;         // burstの生成時間
        bool finishedDestroy = false;   // burstの生成が完了したら自身のGameObjectを破棄する

        // ループ
        int emitRate = 20;      // 1秒あたりの生成量

        // ライフタイム
        float lifetimeMin = 0.5f;   // パーティクルの最小寿命
        float lifetimeMax = 1.0f;   // パーティクルの最大寿命

        // 物理挙動
        Vector3 gravity = Vector3(0.0f, -9.8f, 0.0f);   // 重力加速度
        float drag = 0.1f;                              // 抵抗係数（0.0fで抵抗なし)

        // 見た目
        Vector3 size = Vector3(1.0f, 1.0f, 1.0f);    // パーティクルの初期サイズ(単位:px)
        Curve<float> scaleOverLifetime =                // サイズを変化させるカーブ
            Curve<float>(1.0f, 0.0f);
        Curve<Color> colorOverLifetime =                // 色を変化させるカーブ
            Curve<Color>(Color::White(), Color(1.0f, 1.0f, 1.0f, 0.0f));    

        // 速度共通
        bool randomizeSpeed = true; // true:初速をランダム化する
        float speed = 5.0f;         // パーティクルの初速
        float speedMin = 3.0f;      // パーティクルの初速の最小値
        float speedMax = 8.0f;      // パーティクルの初速の最大値

        // 速度タイプ
        VelocityType velocityType = VelocityType::Directional;

        // Directional / Cone 
        Vector3 direction = Vector3::Up(); 

        // Cone
        float coneAngle = 30.0f;    // Coneの角度（degrees）

        // 生成タイプ
        SpawnType spawnType = SpawnType::Point;

        // Circle / Sphere
        float spawnRadius = 0.1f;   // 生成時の半径

        // Circle
        Vector3 spawnNormal = Vector3::Up(); // 生成時の法線方向
    };
private:
    class Particle
    {
    public:
        bool active{ false };

        Vector3 position{ Vector3::Zero() };
        Vector3 rotation{ Vector3::Zero() };
        float scale{ 1.0f };
        Vector3 velocity{ Vector3::Zero() };
        Color color{ Color::White() };
        float lifetime{ 1.0f };
        float age{ 0.0f };
    };

public :

    ParticleEmitter(GameObject* owner): Component(owner)
    {
    }
    void Update() override;
    std::vector<Particle>& GetParticles() { return m_particles; }

    void SetSettings(const EmitterSettings& settings)
    {
        m_settings = settings;

        m_emitTimer = 0.0f;
        m_burstEmitted = 0;

        if (m_settings.burst)
        {
            if (m_settings.burstCount > 0 && m_settings.burstTime > 0.0f)
                m_emitInterval = m_settings.burstTime / static_cast<float>(m_settings.burstCount);
            else
                m_emitInterval = 0.0f;
        }
        else
        {
            if (m_settings.emitRate > 0)
                m_emitInterval = 1.0f / static_cast<float>(m_settings.emitRate);
            else
                m_emitInterval = 0.0f;
        }

        m_settings.direction.Normalize();
        m_settings.spawnNormal.Normalize();
    }

    const EmitterSettings& GetSettingsRef() const { return m_settings; }
    EmitterSettings GetSettings() { return m_settings; }

    void ResetEmitter()
    {
        m_emitTimer = 0.0f;
        m_burstEmitted = 0;
        m_burstFinished = false;
        for (auto& p : m_particles)
        {
            p.active = false;
        }
    }

    void ResetBurst()
    {
        m_emitTimer = 0.0f;
        m_burstEmitted = 0;
        m_burstFinished = false;
    }

private:
    std::vector<Particle> m_particles;

    EmitterSettings m_settings;
    float m_emitTimer{ 0.0f };
    float m_emitInterval{ 0.1f };
    int m_burstEmitted{ 0 }; // バースト時の生成済みパーティクル数
    bool m_burstFinished{ false }; // バーストが完了したかどうか

    void EmitOne();
    void ActivateParticle(Particle& p);
    void UpdateParticle(Particle& p, float dt);
};

class ParticleRenderer : public Quad
{
private:
    ParticleEmitter* m_emitter;
public:
    ParticleRenderer() = delete;
    ParticleRenderer(
        GameObject* owner,
        ParticleEmitter* emitter,
        const wchar_t* textureFile,
        const char* vertexShaderFile = DEFAULT_VERTEX_SHADER_FILE,
        const char* pixelShaderFile = DEFAULT_PIXEL_SHADER_FILE
        )
        : Quad(owner,textureFile,vertexShaderFile, pixelShaderFile),
        m_emitter(emitter)
    {
        m_blendMode = BlendMode::Additive;
        m_billboardType = BillboardType::CameraFacing;
        m_renderPass = RenderPass::Transparent;
    }

    void Draw() override;
};
