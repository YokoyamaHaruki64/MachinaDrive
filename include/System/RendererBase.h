#pragma once

#include "System/main.h"
#include "System/Component.h"
#include "System/manager.h"
#include "System/renderer.h"
#include "Graphics/GraphicInc.h"

class Camera;

enum class RenderPass
{
    Opaque = 0,
    Transparent = 1,
    Overlay = 2
};


class RendererBase : public Component
{
public:
    enum BillboardType
    {
        None = 0,
        CameraFacing = 1, // カメラに常に向く
        YAxisFixed = 2    // Y軸を固定してカメラに向く
    };


protected:
    RenderPass m_renderPass = RenderPass::Opaque; // 描画パス
    BlendMode m_blendMode = BlendMode::AlphaBlend; // ブレンドモード
    CullMode m_cullMode = CullMode::Back; // カリングモード
    int m_drawLayer = 0; // 描画順序
    BillboardType m_billboardType = BillboardType::None; // ビルボードの種類

    MATERIAL m_material = {
        {0.2f, 0.2f, 0.2f, 1.0f}, // Ambient
        {1.0f, 1.0f, 1.0f, 1.0f}, // Diffuse
        {1.0f, 1.0f, 1.0f, 1.0f}, // Specular
        {0.0f, 0.0f, 0.0f, 1.0f}, // Emission
        32.0f,                    // Shininess
        TRUE                      // TextureEnable
    };

    const XMMATRIX CalcWorldMatrix();

private:
    uint64_t BuildSortKey(const Camera* camera) const;

public:
    RendererBase() = default;
    explicit RendererBase(GameObject* owner);
    virtual ~RendererBase();
    virtual void Draw() override = 0;
    void Submit();
    void BindState();

    int GetDrawLayer() const { return m_drawLayer; }
    void SetDrawLayer(int layer) { m_drawLayer = layer; }

    RenderPass GetRendererPass() const { return m_renderPass; }
    void SetRenderPass(RenderPass pass) { m_renderPass = pass; }


    BillboardType GetBillboardType() const { return m_billboardType; }
    void SetBillboardType(BillboardType type) { m_billboardType = type; }

    MATERIAL GetMaterial() const { return m_material; }
    void SetMaterial(const MATERIAL& material) { m_material = material; }

    BlendMode GetBlendMode() const { return m_blendMode; }
    void SetBlendMode(BlendMode mode) { m_blendMode = mode; }

    CullMode GetCullMode() const { return m_cullMode; }
    void SetCullMode(CullMode mode) { m_cullMode = mode; }

    static const XMMATRIX CalcWorldMatrix(const Transform* transform, BillboardType billboardType);
    static const XMMATRIX CalcWorldMatrix(
        Vector3 position,
        Vector3 rotation,
        Vector3 scale,
        BillboardType billboardType);
};
