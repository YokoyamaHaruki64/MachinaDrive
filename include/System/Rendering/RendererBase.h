#pragma once

#include "System/Core/main.h"
#include "System/Core/component.h"
#include "System/Core/manager.h"
#include "System/Rendering/renderer.h"
#include "Graphics/GraphicInc.h"
#include "Graphics/MaterialPropertyOverride.h"


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
    std::vector<MaterialPropertyOverride> m_materialPropertyOverrides;

    const XMMATRIX CalcWorldMatrix();
    void RegisterFields(FieldRegistry& fields) override;

private:
    SortDescriptor CreateSortDescriptor(const Camera* camera);
    static uint64_t BuildSortKey(const SortDescriptor& descriptor);

public:
    RendererBase() = default;
    explicit RendererBase(GameObject* owner);
    virtual ~RendererBase();
    virtual void Draw() override = 0;
    virtual Material* GetMaterial(uint32_t slot = 0) = 0;
    virtual void SetMaterial(uint32_t slot, Material* material) = 0;
    virtual uint32_t GetMaterialSlotCount() const { return 1; }
    virtual uint32_t GetMeshID() const { return 0; }
    MaterialPropertyOverride& GetMaterialPropertyOverride(uint32_t slot = 0)
    {
        if (m_materialPropertyOverrides.size() <= slot)
        {
            m_materialPropertyOverrides.resize(static_cast<size_t>(slot) + 1);
        }
        return m_materialPropertyOverrides[slot];
    }
    const MaterialPropertyOverride* FindMaterialPropertyOverride(uint32_t slot = 0) const
    {
        if (slot >= m_materialPropertyOverrides.size() || m_materialPropertyOverrides[slot].Empty())
        {
            return nullptr;
        }
        return &m_materialPropertyOverrides[slot];
    }
    void ClearMaterialPropertyOverride(uint32_t slot = 0)
    {
        if (slot < m_materialPropertyOverrides.size())
        {
            m_materialPropertyOverrides[slot].Clear();
        }
    }
    bool IsOwnerStarted() const
    {
        if (!m_owner)
        {
            return false;
        }

        return (m_owner->IsStarted() || Manager::GetGameMode() == Manager::GameMode::Edit) &&
            m_owner->IsActive() &&
            IsEnabled();
    }
    virtual void Submit();

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
