#include "pch.h"

#include "Component/UIImage.h"

#include "Component/UICanvas.h"
#include "Component/UITransform.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/Core/GameObject.h"
#include "System/Core/input.h"
#include "System/Editor/EditorSystem.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Rendering/renderer.h"
#include "Graphics/GraphicInc.h"

namespace
{
    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateUIVertexBuffer(const std::vector<VertexType>& vertices)
    {
        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = vertices.data();

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        Renderer::GetDevice()->CreateBuffer(&description, &initialData, buffer.GetAddressOf());
        return buffer;
    }
}

UIImage::UIImage(GameObject* owner)
    : RendererBase(owner)
{
    m_renderPhase = RenderPhase::Overlay;
    // Hierarchyで追加した直後のInspectorにも既定Materialを反映する。
    OnLoaded();
}

void UIImage::OnLoaded()
{
    if (m_materialPath != m_loadedMaterialPath)
    {
        m_material = m_materialPath.empty() ? nullptr : MaterialManager::Load(m_materialPath);
        m_loadedMaterialPath = m_materialPath;
    }
    if (m_texturePath != m_loadedTexturePath)
    {
        m_texture = m_texturePath.empty() ? TextureHandle{} : TextureManager::LoadTexture(m_texturePath);
        m_loadedTexturePath = m_texturePath;
        if (m_owner->GetUITransform()->IsAspectRatioLocked())
            UpdateSizeFromTexture();
    }
    if (!m_positionBuffer)
    {
        CreateVertexBuffers();
    }
}

void UIImage::UpdateSizeFromTexture()
{
    UINT width{};
    UINT height{};
    UITransform* const transform = m_owner->GetUITransform();
    if (!transform || !TextureManager::GetTextureSize(m_texture, width, height) || width == 0 || height == 0)
    {
        return;
    }

    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    transform->SetAspectRatio(aspectRatio);

    Vector2 stretch = transform->GetStretch();
    if (stretch.x <= 0.0f)
    {
        stretch.x = 100.0f / 1920.0f;
    }
    stretch.y = stretch.x / aspectRatio;
    transform->SetStretch(stretch);
}

void UIImage::OnEditorUpdate()
{
    OnLoaded();
}

bool UIImage::ContainsPointer() const
{
    const UITransform* const transform = m_owner->GetUITransform();
    return transform && transform->GetCanvas() && transform->GetCanvas()->IsPointerTarget(*transform);
}

void UIImage::Update()
{
    OnLoaded();

    m_pointerPressed = false;
    m_pointerReleased = false;
    m_dragStarted = false;
    m_dragEnded = false;

    if (!m_raycastTarget)
    {
        m_pointerHeld = false;
        m_dragging = false;
        m_pointerCaptured = false;
        return;
    }

    if (Input::GetMouseTrigger(Input::MouseButton::Left) && ContainsPointer())
    {
        m_pointerPressed = true;
        m_pointerHeld = true;
        m_pointerCaptured = true;
        m_pressPosition = Input::GetMousePosition();
    }

    if (m_pointerCaptured && Input::GetMousePress(Input::MouseButton::Left))
    {
        m_pointerHeld = true;
        if (m_dragEnabled && !m_dragging && Vector2::DistanceSqr(m_pressPosition, Input::GetMousePosition()) > 4.0f)
        {
            m_dragStarted = true;
            m_dragging = true;
        }
    }

    if (m_pointerCaptured && Input::GetMouseRelease(Input::MouseButton::Left))
    {
        m_pointerReleased = true;
        m_dragEnded = m_dragging;
        m_pointerHeld = false;
        m_dragging = false;
        m_pointerCaptured = false;
    }
}

void UIImage::CreateVertexBuffers()
{
    m_positionBuffer = CreateUIVertexBuffer(std::vector<PositionVertex>{
        { Vector3(-0.5f, -0.5f, 0.0f) }, { Vector3(0.5f, -0.5f, 0.0f) },
        { Vector3(-0.5f, 0.5f, 0.0f) }, { Vector3(0.5f, 0.5f, 0.0f) },
    });
    m_texCoordBuffer = CreateUIVertexBuffer(std::vector<PrimaryTexCoordVertex>{
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
    });
    m_colorBuffer = CreateUIVertexBuffer(std::vector<ColorVertex>(4, { Color::White() }));
}

void UIImage::Draw()
{
#if GM_ENABLE_EDITOR
    if (!Manager::IsPlayMode() && !EditorSystem::IsUIDisplayEnabled())
    {
        return;
    }
#endif
    if (!m_material || !m_positionBuffer)
    {
        return;
    }

    const XMMATRIX originalView = Renderer::GetViewMatrix();
    const XMMATRIX originalProjection = Renderer::GetProjectionMatrix();
    Renderer::SetWorldViewProjection2D();
    UITransform* const uiTransform = m_owner->GetUITransform();
    if (!uiTransform || !uiTransform->GetCanvas())
    {
        Renderer::SetViewMatrix(originalView);
        Renderer::SetProjectionMatrix(originalProjection);
        return;
    }
    Renderer::SetWorldMatrix(uiTransform->GetWorldMatrix());

    MaterialPropertyOverride& overrideProperties = GetMaterialPropertyOverride();
    overrideProperties.SetColor("_BaseColor", m_color);
    m_material->Bind(0, &overrideProperties);

    ID3D11DeviceContext* const context = Renderer::GetDeviceContext();
    ID3D11ShaderResourceView* const texture = TextureManager::GetShaderResourceView(m_texture);
    context->PSSetShaderResources(2, 1, &texture);

    bool hasAllRequiredStreams = true;
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

        if (!vertexBuffer)
        {
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);
        }

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
        {
            hasAllRequiredStreams = false;
            break;
        }
        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    if (hasAllRequiredStreams)
    {
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    Renderer::SetViewMatrix(originalView);
    Renderer::SetProjectionMatrix(originalProjection);
}

Material* UIImage::GetMaterial(uint32_t slot)
{
    OnLoaded();
    return slot == 0 ? m_material : nullptr;
}

void UIImage::SetMaterial(uint32_t slot, Material* material)
{
    if (slot != 0)
    {
        return;
    }

    m_material = material;
    m_materialPath = material ? material->GetFilePath() : std::string{};
    m_loadedMaterialPath = m_materialPath;
}

bool UIImage::SetTexturePath(std::string path)
{
    m_texturePath = std::move(path);
    m_texture = m_texturePath.empty() ? TextureHandle{} : TextureManager::LoadTexture(m_texturePath);
    m_loadedTexturePath = m_texturePath;
    UpdateSizeFromTexture();
    return m_texture.IsValid();
}

void UIImage::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_drawLayer);
    if (m_material && !m_material->GetFilePath().empty())
    {
        m_materialPath = m_material->GetFilePath();
    }

    fields.AddPath("image", m_texturePath, "Assets", { ".png", ".jpg", ".jpeg", ".dds", ".tga" });
    GM_FIELD(m_color);
    GM_FIELD(m_raycastTarget);
    GM_FIELD(m_dragEnabled);
    GM_FIELD_TYPE(m_materialPath, InspectorFieldType::Hidden);
}
