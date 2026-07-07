#pragma once

#include "System/main.h"
#include "System/Component.h"
#include "System/manager.h"
#include "System/renderer.h"

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
    int m_drawLayer = 0; // 描画順序
    BillboardType m_billboardType = BillboardType::None; // ビルボードの種類

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
    void SetRendererPass(RenderPass pass) { m_renderPass = pass; }


    BillboardType GetBillboardType() const { return m_billboardType; }
    void SetBillboardType(BillboardType type) { m_billboardType = type; }

    static const XMMATRIX CalcWorldMatrix(const Transform* transform, BillboardType billboardType);
    static const XMMATRIX CalcWorldMatrix(
        Vector3 position,
        Vector3 rotation,
        Vector3 scale,
        BillboardType billboardType);
};
