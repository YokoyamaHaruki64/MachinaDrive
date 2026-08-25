#pragma once

#include "System/Core/Script.h"

class Wall : public Script
{
    REGISTER_COMPONENT(Wall, "Wall");
    Vector3 m_normal = Vector3::Zero(); // 壁の法線ベクトル 
public:
    Wall() = delete;
    explicit Wall(GameObject* owner, const Vector3& normal = Vector3::Zero())
        : Script(owner)
    {
    }
    virtual ~Wall() {}
    virtual void Update() override {};

    void SetNormal(const Vector3& normal) { m_normal = normal; }
    const Vector3& GetNormal() const { return m_normal; }
};
