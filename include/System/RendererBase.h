#pragma once

#include "System/Component.h"

class RendererBase : public Component
{
private:
    int m_drawLayer = 0; // 描画順序
public:
    RendererBase() = default;
    explicit RendererBase(GameObject* owner)
        : Component(owner)
    {
    }
    virtual void Draw() override = 0;

    int GetDrawLayer() const { return m_drawLayer; }
    void SetDrawLayer(int layer) { m_drawLayer = layer; }
};
