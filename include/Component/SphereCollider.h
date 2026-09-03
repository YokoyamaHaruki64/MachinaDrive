#pragma once

#include "Component/Collider.h"
#include "System/Core/ComponentRegistry.h"

// 球は中心点と半径だけで表現できるため、最も単純な基準形状として使用する。
class SphereCollider : public Collider
{
    REGISTER_COMPONENT(SphereCollider, "Sphere Collider");

    float m_radius{ 0.5f };

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    SphereCollider() = delete;
    explicit SphereCollider(GameObject* owner)
        : Collider(owner)
    {
    }

    ShapeType GetShapeType() const override { return ShapeType::Sphere; }

    float GetRadius() const { return m_radius; }
    void SetRadius(float radius);
};
