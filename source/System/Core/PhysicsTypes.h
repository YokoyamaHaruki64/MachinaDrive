#pragma once

#include "System/Core/Physics.h"

#ifndef JPH_USE_AVX2
#define JPH_USE_AVX2
#define JPH_USE_AVX
#define JPH_USE_SSE4_1
#define JPH_USE_SSE4_2
#define JPH_USE_LZCNT
#define JPH_USE_TZCNT
#define JPH_USE_F16C
#define JPH_USE_FMADD
#endif

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class GameObject;
class Rigidbody;

namespace PhysicsDetail
{
    // 接触したBodyだけでなくCompound ShapeのSubShapeもキーに含める。
    struct RawContactKey
    {
        uint32_t body1{};
        uint32_t subShape1{};
        uint32_t body2{};
        uint32_t subShape2{};

        bool operator==(const RawContactKey& other) const
        {
            return body1 == other.body1
                && subShape1 == other.subShape1
                && body2 == other.body2
                && subShape2 == other.subShape2;
        }
    };

    struct RawContactKeyHash
    {
        size_t operator()(const RawContactKey& key) const
        {
            size_t result = key.body1;
            result = result * 31 + key.subShape1;
            result = result * 31 + key.body2;
            result = result * 31 + key.subShape2;
            return result;
        }
    };

    struct BodyRecord
    {
        GameObject* bodyOwner{};
        Rigidbody* rigidbody{};
        std::vector<Collider*> colliders;
        JPH::BodyID bodyID;
        JPH::ShapeRefC shape;
        // Componentから明示通知された変更だけを、次のPhysics::Updateで処理する。
        bool shapeDirty{ true };
        bool settingsDirty{ true };
        bool settingsRequiresRecreate{ true };
        bool velocityDirty{ true };
        bool transformDirty{ true };
    };

    struct ContactState
    {
        // Joltのワーカースレッドから受け取った接触情報を、メインスレッドのイベントへ変換する中間状態。
        BodyRecord* body1{};
        BodyRecord* body2{};
        Collider* collider1{};
        Collider* collider2{};
        Vector3 point{ Vector3::Zero() };
        Vector3 normal{ Vector3::Up() };
        float penetration{};
        bool isTrigger{};
        uint64_t lastSeenStep{};
        bool began{};
    };

    enum class EventType
    {
        Begin,
        Hit,
        End,
    };

    struct DispatchEvent
    {
        EventType type{};
        HitInfo hitInfo{};
    };
}
