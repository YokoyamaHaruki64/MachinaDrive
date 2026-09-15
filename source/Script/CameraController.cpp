#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/CameraController.h"

#include "Script/Player.h"
#include "Script/Pause.h"
#include "Script/Result.h"

namespace
{
    constexpr float SEGMENT_EPSILON = 1.0e-6f;

    bool IntersectSegmentWithBox(
        const Vector3& segmentStart,
        const Vector3& segmentEnd,
        const Vector3& boxCenter,
        const Quaternion& boxRotation,
        const Vector3& halfExtents,
        float& outEnter)
    {
        const Quaternion inverseRotation = boxRotation.Inversed();
        const Vector3 localStart = inverseRotation.RotateVector(segmentStart - boxCenter);
        const Vector3 localDirection = inverseRotation.RotateVector(segmentEnd - segmentStart);

        float enter = 0.0f;
        float exit = 1.0f;

        const auto updateSlab = [&enter, &exit](float start, float direction, float extent)
        {
            if (std::abs(direction) <= SEGMENT_EPSILON)
            {
                return start >= -extent && start <= extent;
            }

            float nearDistance = (-extent - start) / direction;
            float farDistance = (extent - start) / direction;
            if (nearDistance > farDistance)
            {
                std::swap(nearDistance, farDistance);
            }

            enter = std::max(enter, nearDistance);
            exit = std::min(exit, farDistance);
            return enter <= exit;
        };

        if (!updateSlab(localStart.x, localDirection.x, halfExtents.x)
            || !updateSlab(localStart.y, localDirection.y, halfExtents.y)
            || !updateSlab(localStart.z, localDirection.z, halfExtents.z)
            || exit < 0.0f
            || enter > 1.0f)
        {
            return false;
        }

        outEnter = std::clamp(enter, 0.0f, 1.0f);
        return true;
    }
}

FIELDS_BEGIN(CameraController)
    GM_FIELD(m_distance);
    GM_FIELD(m_sensitivity);
    GM_FIELD(m_offset);
    GM_FIELD(m_collisionPadding);
FIELDS_END()

void CameraController::Shake(float duration, float magnitude)
{
    m_isShaking = true;
    m_shakeTimer = 0.0f;
    m_shakeDuration = duration;
    m_shakeMagnitude = magnitude;
}

void CameraController::OnStart()
{
    // シーン内のPlayerコンポーネントを持つ最初のゲームオブジェクトを取得し、そのTransformをターゲットに設定
    GameObject* playerObject = Manager::GetFirstObjectByComponent<Player>();
    if (playerObject != nullptr)
    {
        m_target = playerObject;
    }
    else
    {
        Logger::Error("Playerコンポーネントを持つGameObjectが見つかりません。");
    }
    m_camera = Camera::GetMainCamera();

}

// 物理Raycastを使わず、WallのBoxColliderをOBBとしてカメラの通過を判定する。
Vector3 CameraController::ResolveWallCollision(
    const Vector3& anchorPosition,
    const Vector3& desiredPosition,
    const Vector3& fallbackPosition) const
{
    const Vector3 segment = desiredPosition - anchorPosition;
    if (segment.LengthSqr() <= SEGMENT_EPSILON)
    {
        return desiredPosition;
    }

    const float padding = std::max(m_collisionPadding, 0.0f);
    float nearestEnter = 1.0f;
    bool hasCollision = false;

    for (GameObject* wallObject : Manager::GetObjectsByTag("Wall"))
    {
        if (!wallObject || !wallObject->IsActive())
        {
            continue;
        }

        BoxCollider* wallCollider = wallObject->GetComponent<BoxCollider>();
        Transform* wallTransform = wallObject->GetTransform();
        if (!wallCollider || !wallCollider->IsEnabled() || !wallTransform)
        {
            continue;
        }

        const Vector3 worldScale = wallTransform->GetScale();
        const Vector3 colliderSize = wallCollider->GetSize();
        const Vector3 halfExtents(
            std::abs(worldScale.x * colliderSize.x) * 0.5f + padding,
            std::abs(worldScale.y * colliderSize.y) * 0.5f + padding,
            std::abs(worldScale.z * colliderSize.z) * 0.5f + padding);

        float enter = 1.0f;
        if (IntersectSegmentWithBox(
                anchorPosition,
                desiredPosition,
                wallCollider->GetWorldCenter(),
                wallTransform->GetRotation(),
                halfExtents,
                enter)
            && enter < nearestEnter)
        {
            nearestEnter = enter;
            hasCollision = true;
        }
    }

    if (!hasCollision)
    {
        return desiredPosition;
    }

    // Anchor自身が余白内にある場合は、Anchorへ戻すとEyeとTargetが一致するため直前位置を維持する。
    return nearestEnter <= SEGMENT_EPSILON
        ? fallbackPosition
        : anchorPosition + segment * nearestEnter;
}

void CameraController::LateUpdate()
{
    if (Pause::IsPause())
    {
        return;
    }

    const bool isResultActive = Result::GetResultState() != Result::ResultState::Playing;
    const Vector2 mouseDelta = isResultActive ? Vector2::Zero() : Input::GetMouseRawDelta();
    Vector3 targetPos = m_target ? m_target->GetTransform()->GetPosition() : Vector3::Zero();

    // カメラのターゲットを更新
    if (!m_target)
        m_camera->SetTarget(Vector3::Zero()); // ターゲットがない場合は原点を向く
    else
        m_camera->SetTarget(targetPos + m_offset);
    Vector3 cameraPos = m_camera->transform()->GetPosition();

    if (!isLocked || isResultActive)
    {
        if (!isResultActive)
        {
            m_yaw += mouseDelta.x * m_sensitivity;
            m_pitch += mouseDelta.y * m_sensitivity;

            // ピッチの制限
            const float pitchLimit_min = 10.0f * Math::DEG2RAD;
            const float pitchLimit_max = 60.0f * Math::DEG2RAD;
            m_pitch = Math::Clamp(m_pitch, pitchLimit_min, pitchLimit_max);
        }

        // カメラの位置を計算
        cameraPos =
            targetPos + Vector3(
                m_distance * std::cos(m_pitch) * std::sin(m_yaw),
                m_distance * std::sin(m_pitch),
                m_distance * std::cos(m_pitch) * std::cos(m_yaw));

        const Vector3 previousCameraPosition = cameraPos;
        cameraPos = ResolveWallCollision(
            targetPos + m_offset,
            cameraPos,
            previousCameraPosition);
        m_camera->transform()->SetPosition(cameraPos);
    }

    if (isResultActive)
    {
        return;
    }

    // カメラの揺れ処理
    if(m_isShaking)
    {
        m_shakeTimer += Time::GetDeltaTime();
        if (m_shakeTimer < m_shakeDuration)
        {
            float t = m_shakeTimer / m_shakeDuration;
            float shakeFactor = m_shakeMagnitude * (1.0f - t); // 揺れの強さを時間経過で減衰させる

            Vector3 currentShakeOffset = Vector3(
                Random::Range(-1.0f, 1.0f),
                Random::Range(-1.0f, 1.0f),
                0.0f
            );

            m_shakeOffset = Math::Lerp(m_shakeOffset, currentShakeOffset, 0.2f);

            Vector3 right = Vector3::Cross(Vector3::Up(), (targetPos - cameraPos).Normalized()).Normalized();

            Vector3 shakeOffset =
                right * m_shakeOffset.x * shakeFactor +
                Vector3::Up() * m_shakeOffset.y * shakeFactor;

            const Vector3 shakenCameraPosition = ResolveWallCollision(
                targetPos + m_offset,
                cameraPos + shakeOffset,
                cameraPos);
            const Vector3 appliedShakeOffset = shakenCameraPosition - cameraPos;

            m_camera->transform()->SetPosition(shakenCameraPosition);

            m_camera->SetTarget(targetPos + m_offset + appliedShakeOffset);
        }
        else
        {
            m_isShaking = false; // 揺れ終了
        }
    }
}
