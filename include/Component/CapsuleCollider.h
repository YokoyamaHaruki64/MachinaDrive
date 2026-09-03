#pragma once

#include "Component/Collider.h"
#include "System/Core/ComponentRegistry.h"

// Capsuleの中心線はLocal Y軸に沿う。Heightは両端の半球を含む全高とする。
// つまり中心線の長さはHeight - Radius * 2となり、Heightが直径未満に
// ならないようSetRadius/SetHeightで不変条件を維持する。
class CapsuleCollider : public Collider
{
    REGISTER_COMPONENT(CapsuleCollider, "Capsule Collider");

    float m_radius{ 0.5f };
    float m_height{ 2.0f };

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    CapsuleCollider() = delete;
    explicit CapsuleCollider(GameObject* owner)
        : Collider(owner)
    {
    }

    ShapeType GetShapeType() const override { return ShapeType::Capsule; }

    float GetRadius() const { return m_radius; }
    void SetRadius(float radius);

    float GetHeight() const { return m_height; }
    void SetHeight(float height);
};
