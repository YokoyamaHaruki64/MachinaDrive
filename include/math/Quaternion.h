#pragma once

#include <algorithm>
#include <cmath>

#include "math/Matrix4x4.h"
#include "math/Vector3.h"

struct Quaternion
{
    float x{ 0.0f };
    float y{ 0.0f };
    float z{ 0.0f };
    float w{ 1.0f };

    constexpr Quaternion() = default;
    constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static constexpr Quaternion Identity()
    {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static Quaternion EulerToQuat(const Vector3& eulerRadians)
    {
        return FromXMQuaternion(DirectX::XMQuaternionRotationRollPitchYaw(
            eulerRadians.x,
            eulerRadians.y,
            eulerRadians.z));
    }

    Vector3 QuatToEuler() const
    {
        const Quaternion rotation = Normalized();
        const float pitchSin = 2.0f * (rotation.w * rotation.x - rotation.y * rotation.z);
        const float pitch = std::asin(std::clamp(pitchSin, -1.0f, 1.0f));
        const float yaw = std::atan2(
            2.0f * (rotation.w * rotation.y + rotation.z * rotation.x),
            1.0f - 2.0f * (rotation.x * rotation.x + rotation.y * rotation.y));
        const float roll = std::atan2(
            2.0f * (rotation.w * rotation.z + rotation.x * rotation.y),
            1.0f - 2.0f * (rotation.x * rotation.x + rotation.z * rotation.z));

        return Vector3(pitch, yaw, roll);
    }

    static Quaternion FromAxisAngle(const Vector3& axis, float radians)
    {
        if (axis.LengthSqr() == 0.0f)
        {
            return Identity();
        }

        const Vector3 normalizedAxis = axis.Normalized();
        return FromXMQuaternion(DirectX::XMQuaternionRotationAxis(
            DirectX::XMVectorSet(normalizedAxis.x, normalizedAxis.y, normalizedAxis.z, 0.0f),
            radians));
    }

    static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3::Up())
    {
        if (forward.LengthSqr() == 0.0f)
        {
            return Identity();
        }

        const Vector3 normalizedForward = forward.Normalized();
        Vector3 normalizedUp = up.LengthSqr() == 0.0f ? Vector3::Up() : up.Normalized();

        if (std::abs(Vector3::Dot(normalizedForward, normalizedUp)) > 0.999f)
        {
            normalizedUp = std::abs(normalizedForward.y) < 0.999f ? Vector3::Up() : Vector3::Right();
        }

        const Vector3 right = Vector3::Cross(normalizedUp, normalizedForward).Normalized();
        const Vector3 correctedUp = Vector3::Cross(normalizedForward, right);
        const DirectX::XMMATRIX rotationMatrix(
            DirectX::XMVectorSet(right.x, right.y, right.z, 0.0f),
            DirectX::XMVectorSet(correctedUp.x, correctedUp.y, correctedUp.z, 0.0f),
            DirectX::XMVectorSet(normalizedForward.x, normalizedForward.y, normalizedForward.z, 0.0f),
            DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

        return FromXMQuaternion(DirectX::XMQuaternionRotationMatrix(rotationMatrix));
    }

    static Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t)
    {
        return FromXMQuaternion(DirectX::XMQuaternionSlerp(
            from.ToXMQuaternion(),
            to.ToXMQuaternion(),
            std::clamp(t, 0.0f, 1.0f)));
    }

    Quaternion Normalized() const
    {
        if (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f)
        {
            return Identity();
        }

        return FromXMQuaternion(DirectX::XMQuaternionNormalize(ToXMQuaternion()));
    }

    Quaternion Inversed() const
    {
        return FromXMQuaternion(DirectX::XMQuaternionInverse(Normalized().ToXMQuaternion()));
    }

    Matrix4x4 ToMatrix() const
    {
        return Matrix4x4::FromXMMATRIX(DirectX::XMMatrixRotationQuaternion(Normalized().ToXMQuaternion()));
    }

    Quaternion operator*(const Quaternion& rhs) const
    {
        return FromXMQuaternion(DirectX::XMQuaternionMultiply(ToXMQuaternion(), rhs.ToXMQuaternion()));
    }

    Vector3 RotateVector(const Vector3& vector) const
    {
        const DirectX::XMVECTOR rotated = DirectX::XMVector3Rotate(
            DirectX::XMVectorSet(vector.x, vector.y, vector.z, 0.0f),
            Normalized().ToXMQuaternion());

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, rotated);
        return Vector3(result.x, result.y, result.z);
    }

private:
    DirectX::XMVECTOR ToXMQuaternion() const
    {
        return DirectX::XMVectorSet(x, y, z, w);
    }

    static Quaternion FromXMQuaternion(DirectX::FXMVECTOR value)
    {
        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, value);
        return Quaternion(result.x, result.y, result.z, result.w);
    }
};
