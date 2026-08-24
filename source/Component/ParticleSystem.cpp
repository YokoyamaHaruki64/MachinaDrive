#include "pch.h"

#include "Component/ComponentInc.h"


#include "System/Renderer.h"
#include "Common/Random.h"
#include "Component/ParticleSystem.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderProgram.h"
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
void ParticleEmitter::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_KEY("offset", m_settings.offset);
    GM_FIELD_KEY("maxParticles", m_settings.maxParticles);
    GM_FIELD_KEY("burst", m_settings.burst);
    GM_FIELD_KEY("burstCount", m_settings.burstCount);
    GM_FIELD_KEY("burstTime", m_settings.burstTime);
    GM_FIELD_KEY("finishedDestroy", m_settings.finishedDestroy);
    GM_FIELD_KEY("emitRate", m_settings.emitRate);
    GM_FIELD_KEY("lifetimeMin", m_settings.lifetimeMin);
    GM_FIELD_KEY("lifetimeMax", m_settings.lifetimeMax);
    GM_FIELD_KEY("gravity", m_settings.gravity);
    GM_FIELD_KEY("drag", m_settings.drag);
    GM_FIELD_KEY("size", m_settings.size);
    GM_FIELD_KEY_TYPE("scaleOverLifetime", m_settings.scaleOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_KEY_TYPE("colorOverLifetime", m_settings.colorOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_KEY("randomizeSpeed", m_settings.randomizeSpeed);
    GM_FIELD_KEY("speed", m_settings.speed);
    GM_FIELD_KEY("speedMin", m_settings.speedMin);
    GM_FIELD_KEY("speedMax", m_settings.speedMax);
    GM_FIELD_KEY("direction", m_settings.direction);
    GM_FIELD_KEY("coneAngle", m_settings.coneAngle);
    GM_FIELD_KEY("spawnRadius", m_settings.spawnRadius);
    GM_FIELD_KEY("spawnNormal", m_settings.spawnNormal);
    fields.AddAccessor<int>("velocityType",
        [this] { return static_cast<int>(m_settings.velocityType); },
        [this](int value) { m_settings.velocityType = static_cast<VelocityType>(value); },
        InspectorFieldSpace::All,
        InspectorFieldType::Auto,
        true);
    fields.AddAccessor<int>("spawnType",
        [this] { return static_cast<int>(m_settings.spawnType); },
        [this](int value) { m_settings.spawnType = static_cast<SpawnType>(value); },
        InspectorFieldSpace::All,
        InspectorFieldType::Auto,
        true);
}

ParticleRenderer::ParticleRenderer(GameObject* owner)
    : RendererBase(owner)
{
    m_billboardType = BillboardType::CameraFacing;
    m_renderPhase = RenderPhase::Transparent;
}

void ParticleRenderer::OnStart()
{
    if (!m_emitter)
    {
        m_emitter = GetComponent<ParticleEmitter>();
    }

    ModelAsset* quadModel = ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad);
    if (quadModel && !quadModel->meshes.empty())
        m_mesh = &quadModel->meshes.front();

    m_material = MaterialManager::Load(m_materialPath);
}

void ParticleRenderer::RegisterFields(FieldRegistry& fields)
{
    if (m_material && !m_material->GetFilePath().empty())
    {
        m_materialPath = m_material->GetFilePath();
    }

    RendererBase::RegisterFields(fields);
    GM_FIELD_TYPE(m_materialPath, InspectorFieldType::Hidden);
}

Material* ParticleRenderer::GetMaterial(uint32_t slot)
{
    // Particleは単一Materialのみを持つ。
    return slot == 0 ? m_material : nullptr;
}

void ParticleRenderer::SetMaterial(uint32_t slot, Material* material)
{
    if (slot == 0)
    {
        m_material = material;
        m_materialPath = material ? material->GetFilePath() : std::string{};
        ClearMaterialPropertyOverride();
    }
}

void ParticleRenderer::BindMesh(const XMMATRIX& worldMatrix, const Color& color)
{
    if (!m_mesh || !m_material)
        return;

    Renderer::SetWorldMatrix(worldMatrix);
    MaterialPropertyOverride& propertyOverride = GetMaterialPropertyOverride();
    propertyOverride.SetColor("_BaseColor", color);
    m_material->Bind(0, &propertyOverride);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    for (const InputSlotBinding& binding : m_material->GetShaderPass(0).GetInputSlotBindings())
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
    if (!m_emitter || !m_mesh || !m_material)
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
