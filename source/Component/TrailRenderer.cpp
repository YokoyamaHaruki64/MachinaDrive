#include "pch.h"

#include "Component/ComponentInc.h"


#include "Component/TrailRenderer.h"

#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/renderer.h"

namespace
{
    constexpr size_t TRAIL_VERTEX_COUNT = 1000 * 2;

    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateTrailVertexBuffer()
    {
        D3D11_BUFFER_DESC description{};
        description.Usage = D3D11_USAGE_DYNAMIC;
        description.ByteWidth = sizeof(VertexType) * TRAIL_VERTEX_COUNT;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, nullptr, buffer.GetAddressOf())))
            MessageBoxA(nullptr, "トレイル頂点バッファの作成に失敗しました。", "Trail Renderer Error", MB_OK | MB_ICONERROR);

        return buffer;
    }
}

void TrailRenderer::UpdateSegments()
{
    // 寿命を超えた頂点を先頭から除去し、現在位置との距離が十分ある場合だけ新規頂点を追加する。
    for (TrailSegment& segment : m_segments)
        segment.elapsedTime += Time::GetDeltaTime();

    while (!m_segments.empty() && m_segments.front().elapsedTime > m_trailLifetime)
        m_segments.pop_front();

    const Vector3 currentPosition = transform()->GetPosition() + m_offset;
    if (m_segments.empty() ||
        (currentPosition - m_segments.back().position).LengthSqr() >= m_minSegmentDistance * m_minSegmentDistance)
    {
        if (m_segments.size() >= MAX_SEGMENTS)
            m_segments.pop_front();

        m_segments.emplace_back(currentPosition);
    }
}

void TrailRenderer::CreateVertexStreams()
{
    // Trailは毎フレーム形状が変化するため、Mesh Assetではなく専用の動的Streamを所有する。
    m_positionBuffer = CreateTrailVertexBuffer<PositionVertex>();
    m_texCoordBuffer = CreateTrailVertexBuffer<PrimaryTexCoordVertex>();
    m_colorBuffer = CreateTrailVertexBuffer<ColorVertex>();
}

TrailRenderer::TrailRenderer(GameObject* owner)
    : RendererBase(owner)
{
    m_renderPhase = RenderPhase::Transparent;
}

void TrailRenderer::OnStart()
{
    m_material = MaterialManager::Load(m_materialPath);
    CreateVertexStreams();

}

void TrailRenderer::RegisterFields(FieldRegistry& fields)
{
    if (m_material && !m_material->GetFilePath().empty())
    {
        m_materialPath = m_material->GetFilePath();
    }

    RendererBase::RegisterFields(fields);
    GM_FIELD_TYPE(m_materialPath, InspectorFieldType::Hidden);
    GM_FIELD(m_offset);
    GM_FIELD(m_fixedDirection);
    GM_FIELD(m_trailLifetime);
    GM_FIELD(m_minSegmentDistance);
    GM_FIELD(m_width);
    fields.AddAccessor<int>("faceMode",
        [this] { return static_cast<int>(m_faceMode); },
        [this](int value) { m_faceMode = static_cast<TrailFaceMode>(value); },
        InspectorFieldSpace::All,
        InspectorFieldType::Auto,
        true);
}

Material* TrailRenderer::GetMaterial(uint32_t slot)
{
    // Trailは単一Materialのみを持つ。
    return slot == 0 ? m_material : nullptr;
}

void TrailRenderer::SetMaterial(uint32_t slot, Material* material)
{
    if (slot == 0)
    {
        m_material = material;
        m_materialPath = material ? material->GetFilePath() : std::string{};
        ClearMaterialPropertyOverride();
    }
}

void TrailRenderer::Draw()
{
    UpdateSegments();
    if (!m_material || m_segments.size() < 2)
        return;

    const size_t vertexCount = m_segments.size() * 2;
    std::vector<PositionVertex> positions(vertexCount);
    std::vector<PrimaryTexCoordVertex> texCoords(vertexCount);
    std::vector<ColorVertex> colors(vertexCount);

    const Vector3 cameraPosition = Camera::GetMainCamera()->transform()->GetPosition();
    for (size_t index = 0; index < m_segments.size(); ++index)
    {
        const Vector3& position = m_segments[index].position;
        const Vector3 direction = index == 0
            ? m_segments[1].position - position
            : index == m_segments.size() - 1
                ? position - m_segments[index - 1].position
                : m_segments[index + 1].position - m_segments[index - 1].position;

        Vector3 normalizedDirection = direction;
        normalizedDirection.Normalize();

        Vector3 normal = m_faceMode == TrailFaceMode::CameraFacing
            ? cameraPosition - position
            : m_fixedDirection;
        normal.Normalize();

        Vector3 side = Vector3::Cross(normal, normalizedDirection);
        side.Normalize();

        const float ageRate = m_segments[index].elapsedTime / m_trailLifetime;
        const float halfWidth = m_widthCurve.Evaluate(ageRate) * m_width * 0.5f;
        const Color color = m_colorCurve.Evaluate(ageRate);
        const float v = static_cast<float>(index) / static_cast<float>(m_segments.size() - 1);

        positions[index * 2] = { position + side * halfWidth };
        positions[index * 2 + 1] = { position - side * halfWidth };
        texCoords[index * 2] = { Vector4(0.0f, v, 0.0f, 0.0f) };
        texCoords[index * 2 + 1] = { Vector4(1.0f, v, 0.0f, 0.0f) };
        colors[index * 2] = { color };
        colors[index * 2 + 1] = { color };
    }

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    const auto updateStream = [context]<typename VertexType>(ID3D11Buffer* buffer, const std::vector<VertexType>& vertices)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource{};
        if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
            return false;

        std::memcpy(mappedResource.pData, vertices.data(), sizeof(VertexType) * vertices.size());
        context->Unmap(buffer, 0);
        return true;
    };

    if (!updateStream(m_positionBuffer.Get(), positions) ||
        !updateStream(m_texCoordBuffer.Get(), texCoords) ||
        !updateStream(m_colorBuffer.Get(), colors))
    {
        return;
    }

    Renderer::SetWorldMatrix(XMMatrixIdentity());
    m_material->Bind(0, FindMaterialPropertyOverride());

    for (const InputSlotBinding& binding : m_material->GetShaderPass(0).GetInputSlotBindings())
    {
        ID3D11Buffer* vertexBuffer = nullptr;
        switch (binding.streamType)
        {
        case VertexStreamType::Position:
            vertexBuffer = m_positionBuffer.Get();
            break;

        case VertexStreamType::PrimaryTexCoord:
            vertexBuffer = m_texCoordBuffer.Get();
            break;

        case VertexStreamType::Color:
            vertexBuffer = m_colorBuffer.Get();
            break;
        }

        // Trailが持たないStreamは共通フォールバックで補い、Shaderごとの差異を吸収する。
        if (!vertexBuffer)
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
            return;

        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(static_cast<UINT>(vertexCount), 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
