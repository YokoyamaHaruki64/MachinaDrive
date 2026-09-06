#pragma once

#include "System/Core/PhysicsTypes.h"
#include "math/Quaternion.h"

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/MotionType.h>

namespace PhysicsDetail
{
    JPH::RVec3 ToJoltPosition(const Vector3& value);
    JPH::Quat ToJoltRotation(const Quaternion& value);
    Vector3 FromJoltVector(JPH::Vec3 value);
    Quaternion FromJoltRotation(JPH::Quat value);
    Vector3 FromJoltPosition(JPH::RVec3 value);

    JPH::ShapeRefC CreateShape(const BodyRecord& record);

    JPH::EMotionType GetJoltMotionType(Rigidbody* rigidbody);
    JPH::EAllowedDOFs GetAllowedDOFs(Rigidbody* rigidbody);
}
