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

void CameraController::Update()
{
    if (m_target == nullptr)
    {
        return; // ターゲットが設定されていない場合は更新しない
    }

    m_camera->SetTarget(m_target->GetPosition());

    if(Input::GetKeyPress(Input::KK_Q))
    {
        transform()->Rotate(Vector3(0.0f, -0.02f, 0.0f)); // 左回転
    }
    if(Input::GetKeyPress(Input::KK_E))
    {
        transform()->Rotate(Vector3(0.0f, 0.02f, 0.0f)); // 右回転
    }
    

    // カメラの位置をターゲットから一定の距離と高さに設定
    Vector3 cameraPos = m_target->GetPosition() + Vector3(
        -sinf(transform()->GetRotation().y) * CAMERA_DISTANCE,
        CAMERA_HEIGHT,
        -cosf(transform()->GetRotation().y) * CAMERA_DISTANCE
    );

    m_camera->transform()->SetPosition(cameraPos);
}
