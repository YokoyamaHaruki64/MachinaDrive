#include "pch.h"

#include "System/RendererBase.h"
#include "Component/Transform.h"
#include "System/renderer.h"
#include "System/manager.h"
#include "Component/Camera.h"

const XMMATRIX RendererBase::CalcWorldMatrix()
{
    Transform* t = transform();
    return CalcWorldMatrix(t, m_billboardType);
}
uint64_t RendererBase::BuildSortKey(const Camera* camera) const
{
    uint64_t sortKey = 0;
    sortKey |= static_cast<uint64_t>(m_renderPass) << 56; // 描画パスを上位8ビットに配置

    uint64_t blendModeBits = static_cast<uint64_t>(m_blendMode) & 0x0F; // ブレンドモードを4ビットに制限
    uint64_t cullModeBits = static_cast<uint64_t>(m_cullMode) & 0x0F;   // カリングモードを4ビットに制限

    auto SetBlendAndCullBits = [&sortKey, blendModeBits, cullModeBits](int start) {
        sortKey |= blendModeBits << (start + 4);    // ブレンドモードを44～47ビットに配置
        sortKey |= cullModeBits << start;           // カリングモードを40～43ビットに配置
        };

    // 描画パスに応じてソートキーを構築
    switch (m_renderPass)
    {
    case RenderPass::Opaque:
        // 不透明オブジェクトは描画順序でソートする
        sortKey |= static_cast<uint64_t>(m_drawLayer) << 48; // 描画順序を48～55ビットに配置
        SetBlendAndCullBits(40);
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
            SetBlendAndCullBits(8);
        }
        break;
    case RenderPass::Overlay:
        sortKey |= static_cast<uint64_t>(m_drawLayer) << 48; // 描画順序を48～55ビットに配置
        SetBlendAndCullBits(40);
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

void RendererBase::BindState()
{
    // ブレンドモードや描画パスに応じたGPUステートの設定を行う

    // ブレンドモードの設定
    if( m_blendMode != Renderer::GetCurrentBlendMode())
    {
        Renderer::SetBlendMode(m_blendMode);
    }
    if( m_cullMode != Renderer::GetCurrentCullMode())
    {
        Renderer::SetRasterizerState(m_cullMode);
    }

}

const XMMATRIX RendererBase::CalcWorldMatrix(const Transform* transform, BillboardType billboardType)
{
    Vector3 position = transform->GetPosition();
    Vector3 rotation = transform->GetRotation();
    Vector3 scale = transform->GetScale();
    return CalcWorldMatrix(position, rotation, scale, billboardType);
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
