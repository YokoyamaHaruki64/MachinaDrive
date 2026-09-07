#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/CameraController.h"


#include "Script/Player.h"

FIELDS_BEGIN(CameraController)
    GM_FIELD(m_distance);
    GM_FIELD(m_sensitivity);
FIELDS_END()

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

    Input::ShowCursor(false); // カーソルを非表示にする

}

void CameraController::LateUpdate()
{
    Vector2 mouseDelta = Input::GetMouseDeltaNormalized();
    Vector3 targetPos = m_target ? m_target->GetTransform()->GetPosition() : Vector3::Zero();

    // カメラのターゲットを更新
    if (!m_target)
        m_camera->SetTarget(Vector3::Zero()); // ターゲットがない場合は原点を向く
    else
        m_camera->SetTarget(targetPos);
    
    if (isLocked)
        return;

    m_yaw += mouseDelta.x * m_sensitivity;
    m_pitch -= mouseDelta.y * m_sensitivity;

    // ピッチの制限
    const float pitchLimit = 30.0f * Math::DEG2RAD; // 30度をラジアンに変換
    m_pitch = Math::Clamp(m_pitch, -pitchLimit, pitchLimit);

    // カメラの位置を計算
    Vector3 cameraPos =
        targetPos + Vector3(
            m_distance * std::cos(m_pitch) * std::sin(m_yaw),
            m_distance * std::sin(m_pitch),
            m_distance * std::cos(m_pitch) * std::cos(m_yaw));

    m_camera->transform()->SetPosition(cameraPos);
}

void CameraController::OnDestroy()
{
    Input::ShowCursor(true); // カーソルを表示する
}
