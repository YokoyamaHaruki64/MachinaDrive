#include "pch.h"

#include "System/Debug/DebugDrawer.h"

#if GM_ENABLE_EDITOR

#include "Component/BoxCollider.h"
#include "Component/CapsuleCollider.h"
#include "Component/SphereCollider.h"
#include "Component/Transform.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"
#include "System/Rendering/renderer.h"

#include <array>
#include <cstring>
#include <limits>
#include <vector>

std::vector<Collider*> DebugDrawer::s_colliders;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_vertices;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_unitSphereVertices;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_unitBoxVertices;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_unitLowerHemisphereVertices;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_unitUpperHemisphereVertices;
std::vector<DebugDrawer::DebugVertex> DebugDrawer::s_unitCylinderVertices;
Microsoft::WRL::ComPtr<ID3D11Buffer> DebugDrawer::s_vertexBuffer;
std::size_t DebugDrawer::s_vertexBufferCapacity = 0;
GraphicsShader* DebugDrawer::s_shader = nullptr;
bool DebugDrawer::s_enabled = true;

float DebugDrawer::GetMaximumAbsScale(const Transform* transform)
{
    if (!transform)
    {
        return 1.0f;
    }

    const Vector3 scale = transform->GetScale();
    return std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) });
}

Quaternion DebugDrawer::GetWorldRotation(const Collider& collider)
{
    const Transform* transform = collider.transform();
    return transform ? transform->GetRotation() : Quaternion::Identity();
}

Vector3 DebugDrawer::GetWorldScale(const Collider& collider)
{
    const Transform* transform = collider.transform();
    return transform ? transform->GetScale() : Vector3::One();
}

DirectX::XMMATRIX DebugDrawer::CreateTransformMatrix(
    const Vector3& scale,
    const Quaternion& rotation,
    const Vector3& translation)
{
    // このプロジェクトの行列は行ベクトル（v * M）なので、ShapeのScaleとRotationを
    // 先に適用し、最後にWorld上の位置へ移動する順番で合成する。
    const Quaternion normalizedRotation = rotation.Normalized();
    const DirectX::XMVECTOR quaternion = DirectX::XMVectorSet(
        normalizedRotation.x,
        normalizedRotation.y,
        normalizedRotation.z,
        normalizedRotation.w);
    return DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
        * DirectX::XMMatrixRotationQuaternion(quaternion)
        * DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
}

void DebugDrawer::AddLine(
    std::vector<DebugVertex>& vertices,
    const Vector3& start,
    const Vector3& end)
{
    // LineListは2頂点で1本を表すため、隣り合う2頂点を必ずペアで追加する。
    vertices.push_back({ start });
    vertices.push_back({ end });
}

void DebugDrawer::AddCircle(
    std::vector<DebugVertex>& vertices,
    const Vector3& center,
    const Vector3& basisA,
    const Vector3& basisB,
    float radius)
{
    if (radius <= 0.0f)
    {
        return;
    }

    Vector3 previous = center + basisA * radius;
    for (int index = 1; index <= CIRCLE_SEGMENT_COUNT; ++index)
    {
        const float angle = Math::PI_2 * static_cast<float>(index)
            / static_cast<float>(CIRCLE_SEGMENT_COUNT);
        const Vector3 current = center
            + (basisA * std::cos(angle) + basisB * std::sin(angle)) * radius;
        AddLine(vertices, previous, current);
        previous = current;
    }
}

void DebugDrawer::AppendSphere(const SphereCollider& collider)
{
    const Vector3 center = collider.GetWorldCenter();
    const Quaternion rotation = GetWorldRotation(collider);
    const float radius = std::abs(collider.GetRadius())
        * GetMaximumAbsScale(collider.transform());
    if (radius <= 0.0f)
    {
        return;
    }

    // Sphereは単位球の3大円を一度だけ生成しておき、ここでは均等Scaleと
    // World回転・中心位置を適用する。非一様ScaleはPhysicsと同じ外接球に合わせる。
    AppendTransformed(
        s_unitSphereVertices,
        CreateTransformMatrix(Vector3(radius, radius, radius), rotation, center));
}

void DebugDrawer::AppendBox(const BoxCollider& collider)
{
    const Vector3 center = collider.GetWorldCenter();
    const Quaternion rotation = GetWorldRotation(collider);
    const Vector3 scale = GetWorldScale(collider);
    const Vector3 size = collider.GetSize();

    // 単位Boxは各辺0.5の立方体として保持しているため、Colliderの全サイズを
    // Scaleに渡すと、現在のPhysicsが使う半サイズと一致する。
    const Vector3 boxScale(
        std::abs(size.x * scale.x),
        std::abs(size.y * scale.y),
        std::abs(size.z * scale.z));
    AppendTransformed(
        s_unitBoxVertices,
        CreateTransformMatrix(boxScale, rotation, center));
}

void DebugDrawer::BuildUnitHemisphere(
    std::vector<DebugVertex>& vertices,
    const Vector3& capAxis)
{
    // CapsuleはXZ平面の境界円と、XY/YZ平面の半円弧だけで外形を確認できる。
    // 上半球なら+Y、下半球なら-Yへ進むため、同じ処理で上下を作る。
    AddCircle(vertices, Vector3::Zero(), Vector3::Right(), Vector3::Forward(), 1.0f);

    const float direction = capAxis.y >= 0.0f ? 1.0f : -1.0f;
    const int segmentCount = CIRCLE_SEGMENT_COUNT / 2;
	const float angleStep = Math::PI / static_cast<float>(segmentCount);

    Vector3 previousXY = Vector3::Right();
    Vector3 previousYZ = Vector3::Forward();

    for (int index = 1; index <= segmentCount; ++index)
    {
		const float angle = angleStep * index;

        const float height = direction * std::sin(angle);
        const float radius = std::cos(angle);

        const Vector3 currentXY(radius, height, 0.0f);
        const Vector3 currentYZ(0.0f, height, radius);

        AddLine(vertices, previousXY, currentXY);
        AddLine(vertices, previousYZ, currentYZ);

        previousXY = currentXY;
        previousYZ = currentYZ;
    }
}

void DebugDrawer::BuildUnitShapes()
{
    s_unitSphereVertices.clear();
    s_unitBoxVertices.clear();
    s_unitLowerHemisphereVertices.clear();
    s_unitUpperHemisphereVertices.clear();
    s_unitCylinderVertices.clear();

    // Sphereは単位球の3大円を単位形状として保持する。
    AddCircle(s_unitSphereVertices, Vector3::Zero(), Vector3::Right(), Vector3::Up(), 1.0f);
    AddCircle(s_unitSphereVertices, Vector3::Zero(), Vector3::Up(), Vector3::Forward(), 1.0f);
    AddCircle(s_unitSphereVertices, Vector3::Zero(), Vector3::Forward(), Vector3::Right(), 1.0f);

    // Boxは中心原点、各辺1の単位立方体として保持する。
    std::array<Vector3, 8> corners;
    for (int index = 0; index < static_cast<int>(corners.size()); ++index)
    {
        corners[index] = Vector3(
            (index & 1) ? 0.5f : -0.5f,
            (index & 2) ? 0.5f : -0.5f,
            (index & 4) ? 0.5f : -0.5f);
    }

    // ビットを各軸の符号に対応させ、隣接する頂点だけを結んで12辺を作る。
    for (int index = 0; index < static_cast<int>(corners.size()); ++index)
    {
        if ((index & 1) == 0)
        {
            AddLine(s_unitBoxVertices, corners[index], corners[index | 1]);
        }
        if ((index & 2) == 0)
        {
            AddLine(s_unitBoxVertices, corners[index], corners[index | 2]);
        }
        if ((index & 4) == 0)
        {
            AddLine(s_unitBoxVertices, corners[index], corners[index | 4]);
        }
    }

    BuildUnitHemisphere(s_unitLowerHemisphereVertices, Vector3::Down());
    BuildUnitHemisphere(s_unitUpperHemisphereVertices, Vector3::Up());

    // Cylinderは半径1、中心線の半分の長さ1として保持する。
	const int CYLINDER_SEGMENT_COUNT = 4;
	const float angleStep = Math::PI_2 / static_cast<float>(CYLINDER_SEGMENT_COUNT);

    for (int longitude = 0; longitude < CYLINDER_SEGMENT_COUNT; ++longitude)
    {
		const float angle = angleStep * longitude;

        const Vector3 radial(
            std::cos(angle),
            0.0f,
            std::sin(angle));

        AddLine(
            s_unitCylinderVertices,
            Vector3(radial.x, -1.0f, radial.z),
            Vector3(radial.x, 1.0f, radial.z));
    }
}

void DebugDrawer::AppendTransformed(
    const std::vector<DebugVertex>& vertices,
    DirectX::FXMMATRIX transform)
{
    if (vertices.empty())
    {
        return;
    }

    const std::size_t outputOffset = s_vertices.size();
    s_vertices.resize(outputOffset + vertices.size());

    // 単位形状は入力と出力で別Vectorに分かれているため、DirectXMathの
    // ストリーム変換で全頂点を一括処理できる。個別のpush_backと行列計算を避け、
    // 変換済み頂点だけを最終的な一つのVectorへ追加する。
    DirectX::XMVector3TransformCoordStream(
        reinterpret_cast<DirectX::XMFLOAT3*>(&s_vertices[outputOffset].position),
        sizeof(DebugVertex),
        reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices.front().position),
        sizeof(DebugVertex),
        vertices.size(),
        transform);
}

void DebugDrawer::AppendCapsule(const CapsuleCollider& collider)
{
    const Vector3 center = collider.GetWorldCenter();
    const Quaternion rotation = GetWorldRotation(collider);
    const Vector3 scale = GetWorldScale(collider);
    const float radialScale = std::max(std::abs(scale.x), std::abs(scale.z));
    const float radius = std::abs(collider.GetRadius()) * radialScale;
    const float halfHeight = std::abs(collider.GetHeight() * scale.y) * 0.5f;
    const float halfSegment = std::max(halfHeight - radius, 0.0f);
    const Vector3 axis = rotation.RotateVector(Vector3::Up()).Normalized();

    // 半球の半径と円柱の中心線長は独立して変化するため、単一のCapsule行列ではなく、
    // 上下半球と円柱へそれぞれ別のScaleを適用する。これでheightとradiusを個別に反映できる。
    const Vector3 lowerCenter = center - axis * halfSegment;
    const Vector3 upperCenter = center + axis * halfSegment;
    if (radius > 0.0f)
    {
        const Vector3 hemisphereScale(radius, radius, radius);
        AppendTransformed(
            s_unitLowerHemisphereVertices,
            CreateTransformMatrix(hemisphereScale, rotation, lowerCenter));
        AppendTransformed(
            s_unitUpperHemisphereVertices,
            CreateTransformMatrix(hemisphereScale, rotation, upperCenter));
    }

    // 半球同士が接する場合は円柱長が0になるため、退化した線分を追加しない。
    if (halfSegment > 0.0f)
    {
        AppendTransformed(
            s_unitCylinderVertices,
            CreateTransformMatrix(
                Vector3(radius, halfSegment, radius),
                rotation,
                center));
    }
}

void DebugDrawer::AppendCollider(const Collider& collider)
{
    switch (collider.GetShapeType())
    {
    case Collider::ShapeType::Sphere:
        if (const auto* sphere = dynamic_cast<const SphereCollider*>(&collider))
        {
            AppendSphere(*sphere);
        }
        break;
    case Collider::ShapeType::Box:
        if (const auto* box = dynamic_cast<const BoxCollider*>(&collider))
        {
            AppendBox(*box);
        }
        break;
    case Collider::ShapeType::Capsule:
        if (const auto* capsule = dynamic_cast<const CapsuleCollider*>(&collider))
        {
            AppendCapsule(*capsule);
        }
        break;
    }
}

bool DebugDrawer::EnsureVertexBuffer(std::size_t vertexCount)
{
    if (!Renderer::GetDevice()
        || vertexCount > std::numeric_limits<UINT>::max() / sizeof(DebugVertex))
    {
        return false;
    }
    if (s_vertexBuffer && s_vertexBufferCapacity >= vertexCount)
    {
        return true;
    }

    D3D11_BUFFER_DESC description{};
    description.ByteWidth = static_cast<UINT>(sizeof(DebugVertex) * vertexCount);
    description.Usage = D3D11_USAGE_DYNAMIC;
    description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ComPtr<ID3D11Buffer> vertexBuffer;
    if (FAILED(Renderer::GetDevice()->CreateBuffer(
        &description,
        nullptr,
        vertexBuffer.GetAddressOf())))
    {
        return false;
    }

    s_vertexBuffer = std::move(vertexBuffer);
    s_vertexBufferCapacity = vertexCount;
    return true;
}

void DebugDrawer::Initialize()
{
    s_enabled = true;
    BuildUnitShapes();
    s_shader = ShaderManager::LoadShaderFromFile(DEBUG_COLLIDER_SHADER_PATH);
    if (!s_shader)
    {
        Logger::Warning("Debug用Collider Shaderを読み込めませんでした。");
    }
}

void DebugDrawer::Uninitialize()
{
    // GPUリソースはRenderer::Uninitより前に解放し、Device破棄後のComPtr解放を避ける。
    s_vertexBuffer.Reset();
    s_vertexBufferCapacity = 0;
    s_vertices.clear();
    s_unitSphereVertices.clear();
    s_unitBoxVertices.clear();
    s_unitLowerHemisphereVertices.clear();
    s_unitUpperHemisphereVertices.clear();
    s_unitCylinderVertices.clear();
    s_colliders.clear();
    s_shader = nullptr;
    s_enabled = false;
}

void DebugDrawer::RegisterCollider(Collider* collider)
{
    if (!collider || std::find(s_colliders.begin(), s_colliders.end(), collider) != s_colliders.end())
    {
        return;
    }

    s_colliders.push_back(collider);
}

void DebugDrawer::UnregisterCollider(Collider* collider)
{
    const auto found = std::find(s_colliders.begin(), s_colliders.end(), collider);
    if (found == s_colliders.end())
    {
        return;
    }

    // 登録順は描画結果に意味を持たないため、末尾との交換で高速に削除する。
    *found = s_colliders.back();
    s_colliders.pop_back();
}

void DebugDrawer::Draw()
{
    if (!s_enabled || !s_shader || !s_shader->IsValid() || !Renderer::GetDeviceContext())
    {
        return;
    }

    s_vertices.clear();
    for (Collider* collider : s_colliders)
    {
        if (!collider || !collider->IsEnabled() || !collider->GetOwner())
        {
            continue;
        }

        const GameObject* owner = collider->GetOwner();
        if (!owner->IsActive() || owner->IsDestroyed())
        {
            continue;
        }

        // 頂点は毎フレーム作り直す。Transformが動いたColliderをキャッシュ更新する必要がなく、
        // EditorのInspector変更とPlay中の移動の両方を同じ経路で描画できる。
        AppendCollider(*collider);
    }

    if (s_vertices.empty() || !EnsureVertexBuffer(s_vertices.size()))
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    if (FAILED(Renderer::GetDeviceContext()->Map(
        s_vertexBuffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &mappedResource)))
    {
        return;
    }

    std::memcpy(
        mappedResource.pData,
        s_vertices.data(),
        sizeof(DebugVertex) * s_vertices.size());
    Renderer::GetDeviceContext()->Unmap(s_vertexBuffer.Get(), 0);

    const GraphicsShaderPass& shaderPass = s_shader->GetShaderPasses().front();
    shaderPass.Bind();

    // Vertex位置はWorld座標で生成済みなので、PerObjectは単位行列にする。
    Renderer::SetWorldMatrix(XMMatrixIdentity());

    ID3D11Buffer* vertexBuffer = s_vertexBuffer.Get();
    const UINT stride = sizeof(DebugVertex);
    const UINT offset = 0;
    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    deviceContext->Draw(static_cast<UINT>(s_vertices.size()), 0);
}

#endif
