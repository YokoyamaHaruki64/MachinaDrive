#include "pch.h"

#include "System/RendererBase.h"
#include "Component/Transform.h"
#include "System/renderer.h"
#include "System/manager.h"
#include "Component/Camera.h"

const XMMATRIX RendererBase::CalcWorldMatrix()
{
    Transform* t = transform();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX scale, rotation, translation;
    // 回転以外の変換行列を作成
    scale = XMMatrixScaling(t->GetScale().x, t->GetScale().y, t->GetScale().z);
    translation = XMMatrixTranslation(t->GetPosition().x, t->GetPosition().y, t->GetPosition().z);
    // ビルボードの種類に応じて回転行列を作成
    switch (m_billboardType)
    {
    case BillboardType::CameraFacing:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        // カメラの回転を抽出してビルボードの回転行列を作成
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        rotation = inverseView;
    }
    break;
    case BillboardType::YAxisFixed:
    {
        XMMATRIX inverseView = XMMatrixInverse(nullptr, Renderer::GetViewMatrix());
        inverseView.r[3] = XMVectorSet(0, 0, 0, 1); // 平行移動成分をゼロにする
        // Y軸を固定して回転行列を作成
        inverseView.r[1] = XMVectorSet(0, 1, 0, 0); // Y軸を固定
        rotation = inverseView;
    }
    break;
    default:
        rotation = XMMatrixRotationRollPitchYaw(t->GetRotation().x, t->GetRotation().y, t->GetRotation().z);
        break;
    }
    world = scale * rotation * translation;
    return world;
}

uint64_t RendererBase::BuildSortKey(const Camera* camera) const
{
    uint64_t sortKey = 0;
    sortKey |= static_cast<uint64_t>(m_renderPass) << 56; // 描画パスを上位8ビットに配置

    // 描画パスに応じてソートキーを構築
    switch (m_renderPass)
    {
    case RenderPass::Opaque:
        // 不透明オブジェクトは描画順序でソートする
        sortKey |= static_cast<uint64_t>(m_drawLayer) << 48; // 描画順序を48～55ビットに配置
        break;

    case RenderPass::Transparent:
        // 描画順序と距離でソートする
        {
            Vector3 camForward = camera->transform()->GetForward();
            Vector3 objToCam = transform()->GetPosition() - camera->transform()->GetPosition();
            float distance = Vector3::Dot(objToCam, camForward); // カメラの前方向に沿った距離を計算


            const float maxDistance = 1000.0f; // 最大距離
            // 距離を0.0～1.0の範囲に正規化
            float normalizedDistance = Math::Clamp(distance / maxDistance, 0.0f, 1.0f);
            // 距離を0~uint32_tの範囲に変換
            uint32_t depth =
                static_cast<uint32_t>(
                    normalizedDistance * std::numeric_limits<uint32_t>::max());

            // Back-to-frontソートのために距離を反転
            uint32_t invDepth =
                std::numeric_limits<uint32_t>::max() - depth;

            sortKey |= static_cast<uint64_t>(m_drawLayer) << 48;    // 描画順序を48～55ビットに配置
            sortKey |= static_cast<uint64_t>(invDepth) << 16;       // 距離を16～47ビットに配置
        }
        break;
    case RenderPass::Overlay:
        sortKey |= static_cast<uint64_t>(m_drawLayer) << 48; // 描画順序を48～55ビットに配置
        break;
    default:
        break;
    }

    return sortKey;
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
    uint64_t sortKey = BuildSortKey(Camera::GetMainCamera());
    DrawCommand cmd = { sortKey, this };
    Renderer::SubmitDrawCommand(cmd);
}
