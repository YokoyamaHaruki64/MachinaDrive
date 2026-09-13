#include "pch.h"

#include "Component/ComponentInc.h"


#include "System/Rendering/renderer.h"
#include "Component/ParticleSystem.h"
#include "Component/VfxLifetime.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderProgram.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

void ParticleEmitter::OnStart()
{
    SetSettings(m_settings);

    for (GameObject* target = m_owner; target; target = target->GetParent())
    {
        if (VfxLifetime* lifetime = target->GetComponent<VfxLifetime>())
        {
            m_vfxLifetime = lifetime;
            lifetime->RegisterEmitter(*this);
            break;
        }
    }
}

void ParticleEmitter::OnDestroy()
{
    NotifyFinished();
}

void ParticleEmitter::ClearVfxLifetime(VfxLifetime* lifetime)
{
    if (m_vfxLifetime == lifetime)
    {
        m_vfxLifetime = nullptr;
    }
}

void ParticleEmitter::NotifyFinished()
{
    if (m_completionNotified)
    {
        return;
    }

    m_completionNotified = true;
    if (m_vfxLifetime)
    {
        m_vfxLifetime->NotifyEmitterFinished(*this);
        m_vfxLifetime = nullptr;
    }
}

void ParticleEmitter::Update()
{
    float dt = Time::GetDeltaTime();

    m_emitTimer += dt;

    if (m_settings.burst)
    {
        if (m_settings.burstCount <= 0)
        {
            m_burstFinished = true;
        }

        if (m_burstEmitted < m_settings.burstCount && !m_burstFinished)
        {
            if (m_emitInterval <= 0.0f)
            {
                // burstTimeが0の場合は、同一フレームで全粒子を生成する。
                while (m_burstEmitted < m_settings.burstCount)
                {
                    EmitOne();
                    m_burstEmitted++;
                }
                m_emitTimer = 0.0f;
            }
            else while (m_emitTimer >= m_emitInterval &&
                m_burstEmitted < m_settings.burstCount)
            {
                m_emitTimer -= m_emitInterval;

                EmitOne();
                m_burstEmitted++;
            }

            m_burstFinished = (m_burstEmitted >= m_settings.burstCount);
        }
        if (m_burstFinished && m_activeParticles == 0)
        {
            NotifyFinished();
            if (m_settings.finishedDestroy)
            {
                m_owner->Destroy();
            }
        }

    }
    else
    {
        while (m_emitInterval > 0.0f && m_emitTimer >= m_emitInterval)
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
    m_completionNotified = false;
    p.age = 0.0f;
    p.lifetime = Random::Range(m_settings.lifetimeMin, m_settings.lifetimeMax);
    p.scale = m_settings.scaleOverLifetime.Evaluate(0.0f);
    p.color = m_settings.colorOverLifetime.Evaluate(0.0f);
    p.emissionColor = m_settings.emissionColor +
        m_settings.emissionColorOverLifetime.Evaluate(0.0f) * m_settings.emissionStrength;
    p.rotation = Random::Range(m_settings.rotationMin, m_settings.rotationMax);
    p.angularVelocity = Random::Range(m_settings.angularVelocityMin, m_settings.angularVelocityMax);
    p.randomSeed = static_cast<uint32_t>(Random::Range(0, std::numeric_limits<int>::max()));
    p.flipbookStartFrame = 0;
    UpdateFlipbookFrame(p);
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
        p.rotation += p.angularVelocity * dt;

        // サイズと色は寿命の正規化値で統一して評価し、Prefab間で時間調整を再利用できるようにする。
        float normalizedAge = p.age / Math::Max(p.lifetime, 0.0001f);
        p.scale = m_settings.scaleOverLifetime.Evaluate(normalizedAge);
        p.color = m_settings.colorOverLifetime.Evaluate(normalizedAge);
        p.emissionColor = m_settings.emissionColor +
            m_settings.emissionColorOverLifetime.Evaluate(normalizedAge) * m_settings.emissionStrength;
        UpdateFlipbookFrame(p);
    }
}

void ParticleEmitter::UpdateFlipbookFrame(Particle& p) const
{
    const int columns = std::max(1, m_settings.flipbookColumns);
    const int rows = std::max(1, m_settings.flipbookRows);
    const int maxFrameCount = static_cast<int>(std::min<int64_t>(
        static_cast<int64_t>(columns) * rows,
        std::numeric_limits<int>::max()));
    const int frameCount = m_settings.flipbookFrameCount > 0
        ? std::min(m_settings.flipbookFrameCount, maxFrameCount)
        : maxFrameCount;

    p.uvScale = Vector2(1.0f / static_cast<float>(columns), 1.0f / static_cast<float>(rows));
    if (frameCount <= 1)
    {
        p.flipbookStartFrame = 0;
        p.uvOffset = Vector2::Zero();
        return;
    }

    if (p.age == 0.0f && m_settings.flipbookRandomStart)
    {
        p.flipbookStartFrame = static_cast<int>(p.randomSeed % static_cast<uint32_t>(frameCount));
    }

    int frame = p.flipbookStartFrame % frameCount;
    if (m_settings.flipbookFrameRate > 0.0f)
    {
        const int advancedFrameCount = static_cast<int>(p.age * m_settings.flipbookFrameRate);
        frame += advancedFrameCount;
        frame = m_settings.flipbookLoop
            ? frame % frameCount
            : std::min(frame, frameCount - 1);
    }

    const int column = frame % columns;
    const int row = frame / columns;
    p.uvOffset = Vector2(p.uvScale.x * column, p.uvScale.y * row);
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
    GM_FIELD_KEY("emissionColor", m_settings.emissionColor);
    GM_FIELD_KEY_TYPE("emissionColorOverLifetime", m_settings.emissionColorOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_KEY("emissionStrength", m_settings.emissionStrength);
    GM_FIELD_KEY("rotationMin", m_settings.rotationMin);
    GM_FIELD_KEY("rotationMax", m_settings.rotationMax);
    GM_FIELD_KEY("angularVelocityMin", m_settings.angularVelocityMin);
    GM_FIELD_KEY("angularVelocityMax", m_settings.angularVelocityMax);
    GM_FIELD_KEY("velocityStretch", m_settings.velocityStretch);
    GM_FIELD_KEY("flipbookColumns", m_settings.flipbookColumns);
    GM_FIELD_KEY("flipbookRows", m_settings.flipbookRows);
    GM_FIELD_KEY("flipbookFrameCount", m_settings.flipbookFrameCount);
    GM_FIELD_KEY("flipbookFrameRate", m_settings.flipbookFrameRate);
    GM_FIELD_KEY("flipbookLoop", m_settings.flipbookLoop);
    GM_FIELD_KEY("flipbookRandomStart", m_settings.flipbookRandomStart);
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

void ParticleRenderer::OnLoaded()
{
    if (!m_emitter)
    {
        m_emitter = GetComponent<ParticleEmitter>();
    }

    ModelAsset* quadModel = ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad);
    if (quadModel && !quadModel->meshes.empty())
        m_mesh = &quadModel->meshes.front();

    if (!m_material && !m_materialPath.empty())
    {
        m_material = MaterialManager::Load(m_materialPath);
    }
}

void ParticleRenderer::OnEditorUpdate()
{
    OnLoaded();
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

bool ParticleRenderer::EnsureInstanceBuffer(uint32_t count)
{
    if (count <= m_instanceCapacity && m_instanceBuffer && m_instanceBufferView)
    {
        return true;
    }

    const uint32_t capacity = std::max(1u, std::bit_ceil(count));
    D3D11_BUFFER_DESC description{};
    description.Usage = D3D11_USAGE_DYNAMIC;
    description.ByteWidth = sizeof(ParticleInstanceData) * capacity;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    description.StructureByteStride = sizeof(ParticleInstanceData);

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, nullptr, buffer.GetAddressOf())))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription{};
    viewDescription.Format = DXGI_FORMAT_UNKNOWN;
    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    viewDescription.BufferEx.FirstElement = 0;
    viewDescription.BufferEx.NumElements = capacity;
    viewDescription.BufferEx.Flags = 0;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bufferView;
    if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(buffer.Get(), &viewDescription, bufferView.GetAddressOf())))
        return false;

    m_instanceCapacity = capacity;
    m_instanceBuffer = std::move(buffer);
    m_instanceBufferView = std::move(bufferView);
    return true;
}

XMMATRIX ParticleRenderer::CalcParticleWorldMatrix(const ParticleEmitter::Particle& particle) const
{
    const ParticleEmitter::EmitterSettings& settings = m_emitter->GetSettingsRef();
    Vector3 scale = Vector3::Multiply(settings.size, transform()->GetScale()) * particle.scale;
    scale.x *= 1.0f + particle.velocity.Length() * settings.velocityStretch;

    if (m_billboardType == BillboardType::CameraFacing)
    {
        const XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        const XMVECTOR cameraRight = XMVector3Normalize(inverseView.r[0]);
        const XMVECTOR cameraUp = XMVector3Normalize(inverseView.r[1]);
        const float projectedX = XMVectorGetX(XMVector3Dot(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&particle.velocity)), cameraRight));
        const float projectedY = XMVectorGetX(XMVector3Dot(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&particle.velocity)), cameraUp));

        const float roll = particle.rotation + (projectedX == 0.0f && projectedY == 0.0f ? 0.0f : atan2f(projectedY, projectedX));
        return XMMatrixScaling(scale.x, scale.y, scale.z) *
            XMMatrixRotationZ(roll) * inverseView *
            XMMatrixTranslation(particle.position.x, particle.position.y, particle.position.z);
    }

    return CalcWorldMatrix(particle.position, Vector3(0.0f, 0.0f, particle.rotation), scale, m_billboardType);
}

void ParticleRenderer::BindLegacyMesh(const XMMATRIX& worldMatrix, const Color& color, const Vector2& uvScale, const Vector2& uvOffset)
{
    Renderer::SetWorldMatrix(worldMatrix);
    MaterialPropertyOverride& propertyOverride = GetMaterialPropertyOverride();
    propertyOverride.SetColor("_BaseColor", color);
    propertyOverride.SetVector2("_UVScale", uvScale);
    propertyOverride.SetVector2("_UVOffset", uvOffset);
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

    std::vector<const ParticleEmitter::Particle*> particles;
    particles.reserve(m_emitter->GetParticles().size());
    for (const ParticleEmitter::Particle& particle : m_emitter->GetParticles())
    {
        if (particle.active)
        {
            particles.push_back(&particle);
        }
    }

    if (m_material->GetShaderPass(0).GetRenderState().blendMode == BlendMode::AlphaBlend)
    {
        const XMMATRIX viewMatrix = Renderer::GetViewMatrix();
        std::stable_sort(particles.begin(), particles.end(), [viewMatrix](const auto* lhs, const auto* rhs)
            {
                const XMVECTOR lhsPosition = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&lhs->position));
                const XMVECTOR rhsPosition = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&rhs->position));
                const float lhsDepth = XMVectorGetZ(XMVector3TransformCoord(lhsPosition, viewMatrix));
                const float rhsDepth = XMVectorGetZ(XMVector3TransformCoord(rhsPosition, viewMatrix));
                return lhsDepth > rhsDepth;
            });
    }

    if (!m_material->GetShaderPass(0).IsInstanced())
    {
        // 既存Prefabの任意Materialはインスタンス入力を持たないため、旧経路で表示互換を維持する。
        for (const ParticleEmitter::Particle* particle : particles)
        {
            BindLegacyMesh(CalcParticleWorldMatrix(*particle), particle->color, particle->uvScale, particle->uvOffset);
            Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Renderer::GetDeviceContext()->DrawIndexed(m_mesh->indexCount, 0, 0);
        }
        return;
    }

    std::vector<ParticleInstanceData> instances;
    instances.reserve(particles.size());
    for (const ParticleEmitter::Particle* particle : particles)
    {
        ParticleInstanceData instance{};
        XMStoreFloat4x4(&instance.world, CalcParticleWorldMatrix(*particle));
        instance.color = XMFLOAT4(particle->color.r, particle->color.g, particle->color.b, particle->color.a);
        instance.emissionColor = XMFLOAT4(
            particle->emissionColor.r,
            particle->emissionColor.g,
            particle->emissionColor.b,
            particle->emissionColor.a);
        instance.uvTransform = XMFLOAT4(
            particle->uvScale.x,
            particle->uvScale.y,
            particle->uvOffset.x,
            particle->uvOffset.y);
        instances.push_back(instance);
    }

    if (instances.empty() || !EnsureInstanceBuffer(static_cast<uint32_t>(instances.size())))
        return;

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    if (FAILED(context->Map(m_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        return;

    std::memcpy(mappedResource.pData, instances.data(), sizeof(ParticleInstanceData) * instances.size());
    context->Unmap(m_instanceBuffer.Get(), 0);

    Renderer::SetWorldMatrix(XMMatrixIdentity());
    m_material->Bind(0, FindMaterialPropertyOverride());
    ID3D11ShaderResourceView* instanceView = m_instanceBufferView.Get();
    context->VSSetShaderResources(2, 1, &instanceView);

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
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexedInstanced(m_mesh->indexCount, static_cast<UINT>(instances.size()), 0, 0, 0);
}
