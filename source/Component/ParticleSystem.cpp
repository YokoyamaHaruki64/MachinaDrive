#include "pch.h"

#include "System/GameObject.h"
#include "System/Renderer.h"
#include "System/GM_Time.h"
#include "Common/Random.h"
#include "Component/ParticleSystem.h"
#include "Component/Transform.h"

#include "Common/Random.h"

void ParticleEmitter::Update()
{
    float dt = Time::GetDeltaTime();

    m_emitTimer += dt;

    if (m_settings.burst)
    {
        if (m_burstEmitted < m_settings.burstCount && !m_burstFinished)
        {
            while (m_emitTimer >= m_emitInterval &&
                m_burstEmitted < m_settings.burstCount)
            {
                m_emitTimer -= m_emitInterval;

                EmitOne();
                m_burstEmitted++;
            }

            m_burstFinished = (m_burstEmitted >= m_settings.burstCount);
            if( m_burstFinished && m_settings.finishedDestroy)
            {
                m_owner->Destroy();
            }
        }
    }
    else
    {
        while (m_emitTimer >= m_emitInterval)
        {
            m_emitTimer -= m_emitInterval;
            EmitOne();
        }
    }

    for (auto& p : m_particles)
    {
        if (!p.active) continue;
        UpdateParticle(p, dt);
    }
}

void ParticleEmitter::EmitOne()
{
    for (auto& p : m_particles)
    {
        if (!p.active)
        {
            ActivateParticle(p);
            break; // 1つのパーティクルを生成したらループを抜ける
        }
    }

    if (m_particles.size() < m_settings.maxParticles)
    {
        Particle newParticle;
        ActivateParticle(newParticle);
        m_particles.push_back(newParticle);
    }

}
void ParticleEmitter::ActivateParticle(Particle& p)
{
    p.active = true;
    p.age = 0.0f;
    p.lifetime = Random::Range(m_settings.lifetimeMin, m_settings.lifetimeMax);
    p.scale = m_settings.scaleOverLifetime.Evaluate(0.0f);
    p.color = m_settings.colorOverLifetime.Evaluate(0.0f);

    // 位置の初期化
    switch (m_settings.spawnType)
    {
    case SpawnType::Point:
        p.position = transform()->GetPosition() + m_settings.offset;
        break;
    case SpawnType::Circle:
    {
        float angle = Random::Range(0.0f, 2.0f * Math::PI);
        float radius = sqrtf(Random::Range(0.0f, 1.0f)) * m_settings.spawnRadius;

        const Vector3& normal = m_settings.spawnNormal;

        Vector3 temp = fabsf(normal.y) < 0.99f
            ? Vector3::Up()
            : Vector3::Right();

        Vector3 right = Vector3::Cross(normal, temp).Normalized();
        Vector3 up = Vector3::Cross(right, normal).Normalized();

        Vector3 randomPos =
            right * (cosf(angle) * radius) +
            up * (sinf(angle) * radius);

        p.position = transform()->GetPosition() + m_settings.offset + randomPos;
        break;
    }
    case SpawnType::Sphere:
    {
        Vector3 randomPos = Random::InsideUnitSphere() * m_settings.spawnRadius;
        p.position = transform()->GetPosition() + m_settings.offset + randomPos;
        break;
    }
    break;
    }


    // 速度の初期化
    float speed = m_settings.speed;
    if (m_settings.randomizeSpeed)
    {
        speed = Random::Range(m_settings.speedMin, m_settings.speedMax);
    }

    switch (m_settings.velocityType)
    {
    case VelocityType::None:
        p.velocity = Vector3::Zero();
        break;
    case VelocityType::Directional:
        p.velocity = m_settings.direction * speed;
        break;
    case VelocityType::Cone:
        {
            Vector3 randomDir = Random::RandomCone(m_settings.direction, m_settings.coneAngle);
            p.velocity = randomDir * speed;
        }
        break;
    case VelocityType::SphereVel:
        {
            Vector3 randomDir = Random::OnUnitSphere();
            p.velocity = randomDir * speed;
        }
        break;
    default:
        p.velocity = Vector3::Zero();
        break;
    }
}

void ParticleEmitter::UpdateParticle(Particle& p, float dt)
{
    p.age += dt;
    if (p.age >= p.lifetime)
    {
        p.active = false; // パーティクルの寿命が尽きたら非アクティブにする
    }
    else
    {
        // 位置の更新
        p.position += p.velocity * dt;
        // 重力
        p.velocity += m_settings.gravity * dt;
        // 抵抗
        p.velocity *= Math::Max(0.0f, 1.0f - m_settings.drag * dt);
        // サイズと色の更新
        float normalizedAge = p.age / p.lifetime;
        p.scale = m_settings.scaleOverLifetime.Evaluate(normalizedAge);
        p.color = m_settings.colorOverLifetime.Evaluate(normalizedAge);
    }
}


void ParticleRenderer::Draw()
{
    const ParticleEmitter::EmitterSettings& settings = m_emitter->GetSettingsRef();
    for (auto& p : m_emitter->GetParticles())
    {
        if (!p.active) continue;

        // 入力レイアウトとシェーダーの設定
        Renderer::GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
        Renderer::GetDeviceContext()->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        Renderer::GetDeviceContext()->PSSetShader(m_pixelShader.Get(), nullptr, 0);

        // ワールド行列の設定

        Renderer::SetWorldMatrix(CalcWorldMatrix(
            p.position,
            p.rotation,
            Vector3::Multiply(settings.size, transform()->GetScale()) * p.scale,
            m_billboardType));

        // マテリアルの設定
        MATERIAL material{};
        material.Diffuse = XMFLOAT4(p.color.r, p.color.g, p.color.b, p.color.a);
        material.TextureEnable = true;
        Renderer::SetMaterial(material);

        // UVの設定
        Renderer::SetUV(XMFLOAT4(m_uvWidth, m_uvHeight, m_uvOffsetX, m_uvOffsetY));

        // 頂点バッファの設定
        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

        // インデックスバッファの設定
        Renderer::GetDeviceContext()->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        // シェーダーリソースビューの設定
        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
        // 描画
        Renderer::GetDeviceContext()->DrawIndexed(m_indexCount, 0, 0);

    }
}
