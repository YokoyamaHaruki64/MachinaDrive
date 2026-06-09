#pragma once

#include "System/Component.h"
#include "System/main.h"

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
    int m_drawLayer = 0; // 描画順序
    BillboardType m_billboardType = BillboardType::None; // ビルボードの種類

    const XMMATRIX CalcWorldMatrix();

public:
    RendererBase() = default;
    explicit RendererBase(GameObject* owner)
        : Component(owner)
    {
    }
    virtual void Draw() override = 0;

    int GetDrawLayer() const { return m_drawLayer; }
    void SetDrawLayer(int layer) { m_drawLayer = layer; }

    BillboardType GetBillboardType() const { return m_billboardType; }
    void SetBillboardType(BillboardType type) { m_billboardType = type; }
};
