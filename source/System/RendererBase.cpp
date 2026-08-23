#include "pch.h"

#include "System/RendererBase.h"
#include "Component/Transform.h"
#include "Graphics/Material.h"
#include "System/renderer.h"
#include "System/manager.h"
#include "Component/Camera.h"

const XMMATRIX RendererBase::CalcWorldMatrix()
{
    Transform* t = transform();
    return CalcWorldMatrix(t, m_billboardType);
}
SortDescriptor RendererBase::CreateSortDescriptor(const Camera* camera)
{
    SortDescriptor descriptor{};
    descriptor.phase = m_renderPhase;
    descriptor.drawLayer = m_drawLayer;
    descriptor.meshID = GetMeshID();

    // Materialを持たないRendererも、予約値0として共通のソート規則を利用する。
    if (Material* material = GetMaterial())
        descriptor.materialID = material->GetID();

    if (m_renderPhase == RenderPhase::Transparent && camera)
    {
        const Vector3 camForward = camera->transform()->GetForward();
        const Vector3 objectToCamera = transform()->GetPosition() - camera->transform()->GetPosition();
        descriptor.depth = Vector3::Dot(objectToCamera, camForward);
    }

    return descriptor;
}

uint64_t RendererBase::BuildSortKey(const SortDescriptor& descriptor)
{
    constexpr int kMinDrawLayer = -32768;
    constexpr int kMaxDrawLayer = 32767;
    constexpr float kMaxSortDistance = 1000.0f;

    const uint64_t drawLayer = static_cast<uint64_t>(
        Math::Clamp(descriptor.drawLayer, kMinDrawLayer, kMaxDrawLayer) - kMinDrawLayer);
    const uint64_t materialID = static_cast<uint64_t>(descriptor.materialID) & 0x00FFFFFFull;
    const uint64_t meshID = static_cast<uint64_t>(descriptor.meshID) & 0x00FFFFFFull;

    switch (descriptor.phase)
    {
    case RenderPhase::Transparent:
    {
        // 透過描画は正しい合成順を優先し、Material/Meshは同一深度内の副キーとする。
        const float normalizedDepth = Math::Clamp(descriptor.depth / kMaxSortDistance, 0.0f, 1.0f);
        const uint64_t inverseDepth = 0xFFFFFFull - static_cast<uint64_t>(normalizedDepth * 0xFFFFFFu);
        return (drawLayer << 48) | (inverseDepth << 24) | ((materialID & 0x0FFFu) << 12) | (meshID & 0x0FFFu);
    }

    case RenderPhase::Overlay:
        // 2Dは描画レイヤーを優先し、同一レイヤーでState変更をまとめる。
        return (drawLayer << 48) | (materialID << 24) | meshID;

    default:
        // 不透明系はMaterialとMeshを連続させ、Shader/Resource/頂点バッファの切替を減らす。
        return (materialID << 40) | (meshID << 16) | drawLayer;
    }
}

RendererBase::RendererBase(GameObject* owner) : Component(owner)
{
    Manager::RegisterRenderer(this);
}

RendererBase::~RendererBase()
{
    Manager::UnregisterRenderer(this);
}

void RendererBase::Submit()
{
    DrawItem item{};
    const SortDescriptor descriptor = CreateSortDescriptor(Camera::GetMainCamera());
    item.sortKey = BuildSortKey(descriptor);
    item.renderer = this;
    item.meshID = descriptor.meshID;
    Renderer::SubmitDrawCommand(m_renderPhase, item);
}

const XMMATRIX RendererBase::CalcWorldMatrix(const Transform* transform, BillboardType billboardType)
{
    if (billboardType == BillboardType::None)
    {
        return transform->GetWorldMatrix().ToXMMATRIX();
    }

    return CalcWorldMatrix(transform->GetPosition(), Vector3::Zero(), transform->GetScale(), billboardType);
}

const XMMATRIX RendererBase::CalcWorldMatrix(
    Vector3 position,
    Vector3 rotation,
    Vector3 scale,
    BillboardType billboardType)
{
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX s, r, t;
    // 回転以外の変換行列を作成
    s = XMMatrixScaling(
        scale.x,
        scale.y,
        scale.z);
    t = XMMatrixTranslation(
        position.x,
        position.y,
        position.z);
    // ビルボードの種類に応じて回転行列を作成
    switch (billboardType)
    {
    case BillboardType::CameraFacing:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        // カメラの回転を抽出してビルボードの回転行列を作成
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        r = inverseView;
    }
    break;
    case BillboardType::YAxisFixed:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        // Y軸を固定して回転行列を作成
        inverseView.r[1] = XMVectorSet(0, 1, 0, 0); // Y軸を固定
        r = inverseView;
    }
    break;
    default:
        r = XMMatrixRotationRollPitchYaw(
            rotation.x,
            rotation.y,
            rotation.z);
        break;
    }
    world = s * r * t;
    return world;
}
