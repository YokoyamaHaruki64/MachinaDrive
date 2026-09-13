#pragma once

#include "System/Core/main.h"

#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"
#include "Component/Transform.h"
#include "Graphics/Material.h"
#include "Graphics/ModelAsset.h"
#include "System/Rendering/RendererBase.h"

#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Curve.h"
#include "Common/Color.h"

#include "Graphics/GraphicInc.h"

#include <algorithm>
#include <wrl/client.h>

class VfxLifetime;

class ParticleEmitter : public Component
{
    REGISTER_COMPONENT(ParticleEmitter, "Particle Emitter");

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
        CurveColor colorOverLifetime =
            CurveColor(Color::White(), Color(1.0f, 1.0f, 1.0f, 0.0f)); // 色を変化させるカーブ
        Color emissionColor = Color::Black();
        CurveColor emissionColorOverLifetime = CurveColor(Color::Black(), Color::Black());
        float emissionStrength = 0.0f;

        float rotationMin = 0.0f;
        float rotationMax = 0.0f;
        float angularVelocityMin = 0.0f;
        float angularVelocityMax = 0.0f;
        float velocityStretch = 0.0f;

        // フリップブック
        int flipbookColumns = 1;       // テクスチャを分割する横方向の数
        int flipbookRows = 1;          // テクスチャを分割する縦方向の数
        int flipbookFrameCount = 0;    // 0の場合は分割された全フレームを使う
        float flipbookFrameRate = 0.0f; // 1秒あたりのフレーム送り数（0で停止）
        bool flipbookLoop = false;
        bool flipbookRandomStart = false;

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
public:
    struct Particle
    {
        bool active{ false };

        Vector3 position{ Vector3::Zero() };
        float rotation{ 0.0f };
        float angularVelocity{ 0.0f };
        float scale{ 1.0f };
        Vector3 velocity{ Vector3::Zero() };
        Color color{ Color::White() };
        Color emissionColor{ Color::Black() };
        uint32_t randomSeed{};
        Vector2 uvScale{ Vector2::One() };
        Vector2 uvOffset{ Vector2::Zero() };
        int flipbookStartFrame{};
        float lifetime{ 1.0f };
        float age{ 0.0f };
    };

public:
    ParticleEmitter(GameObject* owner): Component(owner)
    {
    }
    void OnStart() override;
    void OnDestroy() override;
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
        m_settings.flipbookColumns = std::max(1, m_settings.flipbookColumns);
        m_settings.flipbookRows = std::max(1, m_settings.flipbookRows);
        m_settings.flipbookFrameCount = std::max(0, m_settings.flipbookFrameCount);
        m_settings.flipbookFrameRate = std::max(0.0f, m_settings.flipbookFrameRate);
    }

    const EmitterSettings& GetSettingsRef() const { return m_settings; }
    EmitterSettings GetSettings() { return m_settings; }

    void ResetEmitter()
    {
        m_emitTimer = 0.0f;
        m_burstEmitted = 0;
        m_burstFinished = false;
        m_completionNotified = false;
        for (auto& p : m_particles)
        {
            p.active = false;
        }
        m_activeParticles = 0;
    }

    void ResetBurst()
    {
        m_emitTimer = 0.0f;
        m_burstEmitted = 0;
        m_burstFinished = false;
        m_completionNotified = false;
    }

    void ClearVfxLifetime(VfxLifetime* lifetime);

private:
    std::vector<Particle> m_particles;

    EmitterSettings m_settings;
    float m_emitTimer{ 0.0f };
    float m_emitInterval{ 0.1f };
    int m_burstEmitted{ 0 }; // バースト時の生成済みパーティクル数
    int m_activeParticles{ 0 }; // 現在アクティブなパーティクル数
    bool m_burstFinished{ false }; // バーストが完了したかどうか
    bool m_completionNotified{ false };
    VfxLifetime* m_vfxLifetime{};

    void EmitOne();
    void ActivateParticle(Particle& p);
    void UpdateParticle(Particle& p, float dt);
    void UpdateFlipbookFrame(Particle& p) const;
    void NotifyFinished();

protected:
    void RegisterFields(FieldRegistry& fields) override;
};

class ParticleRenderer : public RendererBase
{
    REGISTER_COMPONENT(ParticleRenderer, "Particle Renderer");

private:
    ParticleEmitter* m_emitter{};
    const Mesh* m_mesh{};
    Material* m_material{};
    std::string m_materialPath{ "Assets/Materials/ParticleInstanced.material" };
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_instanceBufferView;
    uint32_t m_instanceCapacity{};

    struct ParticleInstanceData
    {
        XMFLOAT4X4 world;
        XMFLOAT4 color;
        XMFLOAT4 emissionColor;
        XMFLOAT4 uvTransform;
    };

    bool EnsureInstanceBuffer(uint32_t count);
    XMMATRIX CalcParticleWorldMatrix(const ParticleEmitter::Particle& particle) const;
    void BindLegacyMesh(const XMMATRIX& worldMatrix, const Color& color, const Vector2& uvScale, const Vector2& uvOffset);

public:
    ParticleRenderer() = delete;
    explicit ParticleRenderer(GameObject* owner);

    void OnLoaded() override;
    void OnEditorUpdate() override;
    void Draw() override;
    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
