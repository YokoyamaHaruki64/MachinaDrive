#include "pch.h"

#include "Script/CameraController.h"
#include "System/manager.h"
#include "System/input.h"

#include "Component/Camera.h"
#include "Component/Transform.h"

#include "Script/Player.h"

void CameraController::OnStart()
{
    // シーン内のPlayerコンポーネントを持つ最初のゲームオブジェクトを取得し、そのTransformをターゲットに設定
    GameObject* playerObject = Manager::GetFirstObjectByComponent<Player>();
    if (playerObject != nullptr)
    {
        m_target = playerObject->GetTransform();
    }
    else
    {
        // Playerオブジェクトが見つからない場合のエラーハンドリング（例: デバッグログ）
        OutputDebugStringA("Error: No GameObject with Player component found in the scene.\n");
    }
    m_camera = Camera::GetMainCamera();
}

void CameraController::LateUpdate()
{
    // 回転処理
    if (Input::GetKeyPress(Input::KK_Q))
    {
        transform()->Rotate(Vector3(0.0f, -0.02f, 0.0f)); // 左回転
    }
    if (Input::GetKeyPress(Input::KK_E))
    {
        transform()->Rotate(Vector3(0.0f, 0.02f, 0.0f)); // 右回転
    }

    // カメラのターゲットを設定
    if (m_target == nullptr)
        m_camera->SetTarget(Vector3::Zero()); // ターゲットがない場合は原点を向く
    else
        m_camera->SetTarget(m_target->GetPosition());

    // カメラの位置をターゲットから一定の距離と高さに設定
    Vector3 cameraPos = m_camera->GetTarget() + Vector3(
        -sinf(transform()->GetRotation().y) * CAMERA_DISTANCE,
        CAMERA_HEIGHT,
        -cosf(transform()->GetRotation().y) * CAMERA_DISTANCE
    );

    m_camera->transform()->SetPosition(cameraPos);
}
