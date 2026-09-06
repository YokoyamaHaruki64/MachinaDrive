#include "pch.h"

#include "System/Core/PhysicsJoltUtils.h"

#include "Component/BoxCollider.h"
#include "Component/CapsuleCollider.h"
#include "Component/Rigidbody.h"
#include "Component/SphereCollider.h"
#include "Component/Transform.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <algorithm>
#include <cmath>

namespace PhysicsDetail
{
    JPH::RVec3 ToJoltPosition(const Vector3& value)
    {
        // 現在のプロジェクト設定では軸変換を行わず、位置成分をそのまま境界越しに渡す。
        return JPH::RVec3(value.x, value.y, value.z);
    }

    JPH::Quat ToJoltRotation(const Quaternion& value)
    {
        // 不正な長さのQuaternionがBodyへ入らないよう、境界で正規化する。
        const Quaternion normalized = value.Normalized();
        return JPH::Quat(normalized.x, normalized.y, normalized.z, normalized.w);
    }

    Vector3 FromJoltVector(JPH::Vec3 value)
    {
        return Vector3(value.GetX(), value.GetY(), value.GetZ());
    }

    Quaternion FromJoltRotation(JPH::Quat value)
    {
        return Quaternion(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
    }

    Vector3 FromJoltPosition(JPH::RVec3 value)
    {
        return Vector3(
            static_cast<float>(value.GetX()),
            static_cast<float>(value.GetY()),
            static_cast<float>(value.GetZ()));
    }

    JPH::Ref<JPH::Shape> CreateLeafShape(Collider& collider)
    {
        // JoltのShapeはBodyから見た単位形状として作り、位置・回転はCompound側で与える。
        // 非一様ScaleをJoltのShapeへ直接渡すのではなく、形状寸法へ焼き込む。
        JPH::Ref<JPH::Shape> shape;
        const Vector3 scale = collider.transform()->GetScale();

        if (const BoxCollider* box = dynamic_cast<const BoxCollider*>(&collider))
        {
            const Vector3 size = box->GetSize();
            const Vector3 halfExtent(
                std::max(std::abs(size.x * scale.x) * 0.5f, 0.001f),
                std::max(std::abs(size.y * scale.y) * 0.5f, 0.001f),
                std::max(std::abs(size.z * scale.z) * 0.5f, 0.001f));
            shape = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));
        }
        else if (const SphereCollider* sphere = dynamic_cast<const SphereCollider*>(&collider))
        {
            const float radius = std::max(
                std::abs(sphere->GetRadius())
                    * std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) }),
                0.001f);
            shape = new JPH::SphereShape(radius);
        }
        else if (const CapsuleCollider* capsule = dynamic_cast<const CapsuleCollider*>(&collider))
        {
            const float radius = std::max(
                std::abs(capsule->GetRadius())
                    * std::max(std::abs(scale.x), std::abs(scale.z)),
                0.001f);
            const float halfHeight = std::abs(capsule->GetHeight() * scale.y) * 0.5f;
            const float halfHeightOfCylinder = std::max(halfHeight - radius, 0.001f);
            shape = new JPH::CapsuleShape(halfHeightOfCylinder, radius);
        }

        return shape;
    }

    JPH::ShapeRefC CreateShape(const BodyRecord& record)
    {
        // ColliderなしRigidbodyも、速度・重力・Sleepを持つBodyとして登録する。
        if (record.colliders.empty())
        {
            return JPH::ShapeRefC(new JPH::EmptyShape());
        }

        const Transform* bodyTransform = record.bodyOwner->GetTransform();
        const Vector3 bodyPosition = bodyTransform->GetPosition();
        const Quaternion inverseBodyRotation = bodyTransform->GetRotation().Inversed();
        JPH::StaticCompoundShapeSettings settings;

        for (size_t index = 0; index < record.colliders.size(); ++index)
        {
            Collider* collider = record.colliders[index];
            JPH::Ref<JPH::Shape> leafShape = CreateLeafShape(*collider);
            if (!leafShape)
            {
                continue;
            }

            // CompoundのSubShape UserDataではなく、リーフShapeへ設定する。
            // これにより接触時のCollider逆引きがJoltのRTTIへ依存しない。
            leafShape->SetUserData(static_cast<uint64_t>(index + 1));

            const Transform* colliderTransform = collider->transform();
            const Vector3 relativePosition = inverseBodyRotation.RotateVector(
                collider->GetWorldCenter() - bodyPosition);
            const Quaternion relativeRotation = inverseBodyRotation
                * colliderTransform->GetRotation();
            settings.AddShape(
                JPH::Vec3(relativePosition.x, relativePosition.y, relativePosition.z),
                ToJoltRotation(relativeRotation),
                leafShape.GetPtr());
        }

        JPH::Shape::ShapeResult result = settings.Create();
        if (!result.IsValid())
        {
            Logger::Error("JoltのCompound Shape生成に失敗しました: " + result.GetError());
            return JPH::ShapeRefC(new JPH::EmptyShape());
        }
        return result.Get();
    }

    JPH::EMotionType GetJoltMotionType(Rigidbody* rigidbody)
    {
        // RigidbodyがないColliderは静的Bodyとして生成する。
        if (!rigidbody)
        {
            return JPH::EMotionType::Static;
        }

        switch (rigidbody->GetMotionType())
        {
        case Rigidbody::MotionType::Static:
            return JPH::EMotionType::Static;
        case Rigidbody::MotionType::Kinematic:
            return JPH::EMotionType::Kinematic;
        case Rigidbody::MotionType::Dynamic:
        default:
            return JPH::EMotionType::Dynamic;
        }
    }

    JPH::EAllowedDOFs GetAllowedDOFs(Rigidbody* rigidbody)
    {
        // 初期値では回転を固定し、Playerのような簡単な地上移動で転倒しないようにする。
        if (!rigidbody || !rigidbody->IsFreezeRotation())
        {
            return JPH::EAllowedDOFs::All;
        }
        return JPH::EAllowedDOFs::TranslationX
            | JPH::EAllowedDOFs::TranslationY
            | JPH::EAllowedDOFs::TranslationZ;
    }
}
