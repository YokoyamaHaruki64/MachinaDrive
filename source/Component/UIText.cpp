#include "pch.h"

#include "Component/UIText.h"

#include "Component/UICanvas.h"
#include "Component/UITransform.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/Core/GameObject.h"
#include "System/Core/manager.h"
#include "System/Editor/EditorSystem.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Rendering/renderer.h"
#include "System/UI/FontAsset.h"

namespace
{
    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateTextVertexBuffer(const std::vector<VertexType>& vertices)
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

    bool ReadUTF8CodePoint(const std::string& text, size_t& index, uint32_t& output)
    {
        if (index >= text.size())
        {
            return false;
        }

        const uint8_t first = static_cast<uint8_t>(text[index++]);
        if ((first & 0x80) == 0)
        {
            output = first;
            return true;
        }

        int remaining = (first & 0xE0) == 0xC0 ? 1 : (first & 0xF0) == 0xE0 ? 2 : (first & 0xF8) == 0xF0 ? 3 : -1;
        if (remaining < 0 || index + static_cast<size_t>(remaining) > text.size())
        {
            return false;
        }

        output = first & ((1u << (6 - remaining)) - 1u);
        for (int count = 0; count < remaining; ++count)
        {
            const uint8_t byte = static_cast<uint8_t>(text[index++]);
            if ((byte & 0xC0) != 0x80)
            {
                return false;
            }
            output = (output << 6) | (byte & 0x3F);
        }
        return true;
    }
}

UIText::UIText(GameObject* owner)
    : RendererBase(owner)
{
    m_renderPhase = RenderPhase::Overlay;
    // Hierarchyで追加した直後のInspectorにも既定Materialを反映する。
    OnLoaded();
}

void UIText::OnLoaded()
{
    if (m_materialPath != m_loadedMaterialPath)
    {
        m_material = m_materialPath.empty() ? nullptr : MaterialManager::Load(m_materialPath);
        m_loadedMaterialPath = m_materialPath;
    }
    if (m_fontPath != m_loadedFontPath)
    {
        m_font = m_fontPath.empty() ? nullptr : FontManager::Load(m_fontPath);
        m_loadedFontPath = m_fontPath;
    }
    if (!m_positionBuffer)
    {
        CreateVertexBuffers();
    }
}

void UIText::OnEditorUpdate()
{
    OnLoaded();
}

void UIText::Update()
{
    OnLoaded();
}

void UIText::CreateVertexBuffers()
{
    m_positionBuffer = CreateTextVertexBuffer(std::vector<PositionVertex>{
        { Vector3(-0.5f, -0.5f, 0.0f) }, { Vector3(0.5f, -0.5f, 0.0f) },
        { Vector3(-0.5f, 0.5f, 0.0f) }, { Vector3(0.5f, 0.5f, 0.0f) },
    });
    m_texCoordBuffer = CreateTextVertexBuffer(std::vector<PrimaryTexCoordVertex>{
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
    });
    m_colorBuffer = CreateTextVertexBuffer(std::vector<ColorVertex>(4, { Color::White() }));
}

void UIText::Draw()
{
#if GM_ENABLE_EDITOR
    if (!Manager::IsPlayMode() && !EditorSystem::IsUIDisplayEnabled())
    {
        return;
    }
#endif
    UITransform* const transform = m_owner->GetUITransform();
    if (!m_material || !m_font || !transform || !transform->GetCanvas() || !m_positionBuffer)
    {
        return;
    }

    const float pixelScale = m_fontSize * transform->GetCanvas()->GetScale() / static_cast<float>(m_font->GetLineHeight());
    const UIRect& rect = transform->GetPixelRect();
    float cursorX = rect.min.x;
    float cursorY = rect.min.y;

    const XMMATRIX originalView = Renderer::GetViewMatrix();
    const XMMATRIX originalProjection = Renderer::GetProjectionMatrix();
    Renderer::SetWorldViewProjection2D();
    ID3D11DeviceContext* const context = Renderer::GetDeviceContext();
    ID3D11ShaderResourceView* const texture = TextureManager::GetShaderResourceView(m_font->GetTexture());
    context->PSSetShaderResources(2, 1, &texture);

    size_t index = 0;
    uint32_t codePoint{};
    while (ReadUTF8CodePoint(m_text, index, codePoint))
    {
        if (codePoint == '\n')
        {
            cursorX = rect.min.x;
            cursorY += static_cast<float>(m_font->GetLineHeight()) * pixelScale;
            continue;
        }

        const FontGlyph* const glyph = m_font->FindGlyph(codePoint);
        if (!glyph)
        {
            continue;
        }

        const float width = static_cast<float>(glyph->width) * pixelScale;
        const float height = static_cast<float>(glyph->height) * pixelScale;
        const Vector3 position(
            cursorX + (static_cast<float>(glyph->xOffset) * pixelScale) + width * 0.5f,
            cursorY + (static_cast<float>(glyph->yOffset) * pixelScale) + height * 0.5f,
            0.0f);

        MaterialPropertyOverride properties;
        properties.SetColor("_BaseColor", m_color);
        properties.SetVector2("_UVOffset", Vector2(
            static_cast<float>(glyph->x) / static_cast<float>(m_font->GetTextureWidth()),
            static_cast<float>(glyph->y) / static_cast<float>(m_font->GetTextureHeight())));
        properties.SetVector2("_UVScale", Vector2(
            static_cast<float>(glyph->width) / static_cast<float>(m_font->GetTextureWidth()),
            static_cast<float>(glyph->height) / static_cast<float>(m_font->GetTextureHeight())));
        Renderer::SetWorldMatrix(RendererBase::CalcWorldMatrix(position, Vector3::Zero(), Vector3(width, height, 1.0f), BillboardType::None));
        m_material->Bind(0, &properties);

        const GraphicsShaderPass& pass = m_material->GetShaderPass(0);
        for (const InputSlotBinding& binding : pass.GetInputSlotBindings())
        {
            ID3D11Buffer* vertexBuffer = binding.streamType == VertexStreamType::Position ? m_positionBuffer.Get()
                : binding.streamType == VertexStreamType::PrimaryTexCoord ? m_texCoordBuffer.Get()
                : binding.streamType == VertexStreamType::Color ? m_colorBuffer.Get()
                : Renderer::GetFallbackVertexBuffer(binding.streamType);
            const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
            const UINT offset = 0;
            if (vertexBuffer && stride)
            {
                context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
            }
        }
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
        cursorX += static_cast<float>(glyph->xAdvance) * pixelScale;
    }
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Renderer::SetViewMatrix(originalView);
    Renderer::SetProjectionMatrix(originalProjection);
}

Material* UIText::GetMaterial(uint32_t slot)
{
    OnLoaded();
    return slot == 0 ? m_material : nullptr;
}

void UIText::SetMaterial(uint32_t slot, Material* material)
{
    if (slot == 0)
    {
        m_material = material;
        m_materialPath = material ? material->GetFilePath() : std::string{};
        m_loadedMaterialPath = m_materialPath;
    }
}

void UIText::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_drawLayer);
    fields.AddPath("font", m_fontPath, "Assets", { ".fnt" });
    fields.Add("text", m_text, InspectorFieldType::Multiline);
    GM_FIELD(m_fontSize);
    GM_FIELD(m_color);
    GM_FIELD_TYPE(m_materialPath, InspectorFieldType::Hidden);
}
