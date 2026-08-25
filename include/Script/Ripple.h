#pragma once

#include "System/Core/Script.h"
#include "Common/Color.h"
#include "math/Curve.h"

class ModelRenderer;

class Ripple : public Script
{
    REGISTER_COMPONENT(Ripple, "Ripple");
    ModelRenderer* m_renderer = nullptr;

    float m_lifeTime = 1.0f; // 波紋の寿命
    float m_elapsedTime = 0.0f; // 経過時間

    float baseScale = 0.5f; // 基本スケール

    static constexpr int COLOR_PALLET_SIZE = 5;
    static constexpr Color m_colorPallet[COLOR_PALLET_SIZE] = {
        Color(0.0f, 0.5f, 1.0f, 1.0f), // 青
        Color(0.0f, 1.0f, 0.5f, 1.0f), // 緑
        Color(1.0f, 0.5f, 0.0f, 1.0f), // オレンジ
        Color(1.0f, 0.0f, 0.5f, 1.0f), // ピンク
        Color(1.0f, 1.0f, 0.5f, 1.0f)  // 黄色
    };

    Curve<Color> m_colorCurve{};

public:
    Ripple(GameObject* owner) : Script(owner)
    {}
    virtual ~Ripple() override {}
    virtual void OnStart() override;
    virtual void Update() override;

    void SetLifeTime(float lifeTime) { m_lifeTime = lifeTime; }
    const float GetLifeTime() const { return m_lifeTime; }
};
