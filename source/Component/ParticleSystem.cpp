#include "pch.h"

#include "System/GameObject.h"
#include "System/Renderer.h"
#include "System/GM_Time.h"
#include "Common/Random.h"
#include "Component/ParticleSystem.h"
#include "Component/Transform.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"

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
        }
        if (m_burstFinished &&
            m_settings.finishedDestroy &&
            m_activeParticles == 0)

        {
            m_owner->Destroy();
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
            return;
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
    m_activeParticles++;

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
        m_activeParticles--;
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


ParticleRenderer::ParticleRenderer(
    GameObject* owner,
    ParticleEmitter* emitter,
    const wchar_t* textureFile)
    : RendererBase(owner),
    m_emitter(emitter)
{
    ModelAsset* quadModel = ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad);
    if (quadModel && !quadModel->meshes.empty())
        m_mesh = &quadModel->meshes.front();

    static uint32_t materialInstanceIndex = 0;
    if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitAdditive.shader"))
        m_materialInstance = MaterialManager::Create("Particle." + std::to_string(materialInstanceIndex++), *shader);

    m_texture = textureFile ? TextureManager::LoadTexture(std::wstring(textureFile)) : TextureHandle{};
    m_billboardType = BillboardType::CameraFacing;
    m_renderPhase = RenderPhase::Transparent;
}

Material* ParticleRenderer::GetMaterial(uint32_t slot)
{
    // Particleは単一Materialのみを持つ。
    return slot == 0 ? m_materialInstance : nullptr;
}

void ParticleRenderer::BindMesh(const XMMATRIX& worldMatrix, const Color& color) const
{
    if (!m_mesh || !m_materialInstance)
        return;

    Renderer::SetWorldMatrix(worldMatrix);
    m_materialInstance->SetColor("_BaseColor", color);
    m_materialInstance->SetTexture("_MainTex", m_texture);
    m_materialInstance->Bind(0);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    for (const InputSlotBinding& binding : m_materialInstance->GetShaderPass(0).GetInputSlotBindings())
    {
        ID3D11Buffer* vertexBuffer = m_mesh->GetVertexBuffer(binding.streamType);
        if (!vertexBuffer)
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
            return;

        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    context->IASetIndexBuffer(m_mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void ParticleRenderer::Draw()
{
    if (!m_emitter || !m_mesh || !m_materialInstance)
        return;

    const ParticleEmitter::EmitterSettings& settings = m_emitter->GetSettingsRef();
    for (auto& p : m_emitter->GetParticles())
    {
        if (!p.active) continue;

        // Particleごとの座標と色をMaterialへ設定し、共有するQuad形状を描画する。
        BindMesh(CalcWorldMatrix(
            p.position,
            p.rotation,
            Vector3::Multiply(settings.size, transform()->GetScale()) * p.scale,
            m_billboardType),
            p.color);

        Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Renderer::GetDeviceContext()->DrawIndexed(m_mesh->indexCount, 0, 0);

    }
}
