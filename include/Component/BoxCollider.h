#pragma once

#include "Component/Collider.h"
#include "System/Core/ComponentRegistry.h"

// BoxはLocal軸と半サイズからWorld上のOBBとして判定する。
// そのため、Transformの回転を反映した箱同士でもSATで扱える。
class BoxCollider : public Collider
{
    REGISTER_COMPONENT(BoxCollider, "Box Collider");

    Vector3 m_size{ Vector3::One() };

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    BoxCollider() = delete;
    explicit BoxCollider(GameObject* owner)
        : Collider(owner)
    {
    }

    ShapeType GetShapeType() const override { return ShapeType::Box; }

    const Vector3& GetSize() const { return m_size; }
    void SetSize(const Vector3& size);
};
