#include "pch.h"

#include "Component/ComponentInc.h"
#include "Common/Random.h"

#include "Script/CameraController.h"


#include "Script/Player.h"

FIELDS_BEGIN(CameraController)
    GM_FIELD(m_distance);
    GM_FIELD(m_sensitivity);
    GM_FIELD(m_offset);
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

void CameraController::LateUpdate()
{
    Vector2 mouseDelta = Input::GetMouseRawDelta();
    Vector3 targetPos = m_target ? m_target->GetTransform()->GetPosition() : Vector3::Zero();

    // カメラのターゲットを更新
    if (!m_target)
        m_camera->SetTarget(Vector3::Zero()); // ターゲットがない場合は原点を向く
    else
        m_camera->SetTarget(targetPos + m_offset);
    Vector3 cameraPos = m_camera->transform()->GetPosition();

    if (!isLocked)
    {

        m_yaw += mouseDelta.x * m_sensitivity;
        m_pitch += mouseDelta.y * m_sensitivity;

        // ピッチの制限
        const float pitchLimit_min = 10.0f * Math::DEG2RAD;
        const float pitchLimit_max = 30.0f * Math::DEG2RAD;
        m_pitch = Math::Clamp(m_pitch, pitchLimit_min, pitchLimit_max);

        // カメラの位置を計算
        cameraPos =
            targetPos + Vector3(
                m_distance * std::cos(m_pitch) * std::sin(m_yaw),
                m_distance * std::sin(m_pitch),
                m_distance * std::cos(m_pitch) * std::cos(m_yaw));

        m_camera->transform()->SetPosition(cameraPos);
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

            m_camera->transform()->SetPosition(cameraPos + shakeOffset);

            m_camera->SetTarget(targetPos + m_offset + shakeOffset);
        }
        else
        {
            m_isShaking = false; // 揺れ終了
        }
    }
}
