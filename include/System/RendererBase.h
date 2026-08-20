#pragma once

#include "System/main.h"
#include "System/Component.h"
#include "System/manager.h"
#include "System/renderer.h"
#include "Graphics/GraphicInc.h"

class Camera;

struct SortDescriptor
{
    RenderPhase phase{};
    MaterialID materialID{};
    uint32_t meshID{};
    int drawLayer{};
    float depth{};
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
    RenderPhase m_renderPhase = RenderPhase::Opaque;
    int m_drawLayer = 0; // 描画順序
    BillboardType m_billboardType = BillboardType::None; // ビルボードの種類

    const XMMATRIX CalcWorldMatrix();

private:
    SortDescriptor CreateSortDescriptor(const Camera* camera);
    static uint64_t BuildSortKey(const SortDescriptor& descriptor);

public:
    RendererBase() = default;
    explicit RendererBase(GameObject* owner);
    virtual ~RendererBase();
    virtual void Draw() override = 0;
    virtual Material* GetMaterial(uint32_t slot = 0) = 0;
    virtual uint32_t GetMeshID() const { return 0; }
    bool IsOwnerStarted() const { return m_owner && m_owner->IsStarted(); }
    void Submit();

    int GetDrawLayer() const { return m_drawLayer; }
    void SetDrawLayer(int layer) { m_drawLayer = layer; }

    RenderPhase GetRenderPhase() const { return m_renderPhase; }
    void SetRenderPhase(RenderPhase phase) { m_renderPhase = phase; }


    BillboardType GetBillboardType() const { return m_billboardType; }
    void SetBillboardType(BillboardType type) { m_billboardType = type; }

    static const XMMATRIX CalcWorldMatrix(const Transform* transform, BillboardType billboardType);
    static const XMMATRIX CalcWorldMatrix(
        Vector3 position,
        Vector3 rotation,
        Vector3 scale,
        BillboardType billboardType);
};
