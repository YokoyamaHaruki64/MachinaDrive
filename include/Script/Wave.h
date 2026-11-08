#pragma once

#include "System/Core/Script.h"

class Wave : public Script
{
    GM_SCRIPT(Wave, "Wave");
    USE_FIELDS

    int m_division = 10;
    ModelAsset* m_mesh = nullptr;
    ModelRenderer* m_meshRenderer = nullptr;

    float m_amplitude1 = 0.13f;
    float m_amplitude2 = 0.23f;
    float m_amplitude3 = 0.31f;

    float m_frequency1 = 0.77f;
    float m_frequency2 = 1.03f;
    float m_frequency3 = 1.31f;

    Vector2 m_direction1 = Vector2(1.0f, 0.2f);
    Vector2 m_direction2 = Vector2(-0.4f, 0.9f);
    Vector2 m_direction3 = Vector2(0.7f, -0.8f);

    float m_speed1 = 1.23f;
    float m_speed2 = 2.0f;
    float m_speed3 = 0.21f;

    float m_textureScale = 1.0f;

public:
    void OnStart() override;
    void Update() override;
};
