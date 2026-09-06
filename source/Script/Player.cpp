#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"
#include "Scene/ResultScene.h"
#include "math/MathCommon.h"
#include "Script/Bullet.h"
#include "Script/Enemy.h"

void Player::RegisterFields(FieldRegistry& fields)
{
    // 旧シーンを読み込めるように残すが、実行時の物理値はRigidbodyへ集約する。
    GM_FIELD_TYPE(m_velocity, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_isGrounded, InspectorFieldType::Hidden);
}

void Player::OnStart()
{
    m_jumpSE = m_owner->GetComponent<Audio>();

    // 旧Player実装で保存された初速だけは、Rigidbodyへ一度だけ移行する。
    if (Rigidbody* rigidbody = m_owner->GetComponent<Rigidbody>();
        rigidbody && rigidbody->GetVelocity().LengthSqr() == 0.0f && m_velocity.LengthSqr() > 0.0f)
    {
        rigidbody->SetVelocity(m_velocity);
    }
}

void Player::Update()
{
    Rigidbody* rigidbody = m_owner->GetComponent<Rigidbody>();
    if (!rigidbody)
    {
        return;
    }

    Camera* camera = Camera::GetMainCamera();
    Vector3 cameraDir = camera->GetTarget() - camera->transform()->GetPosition();
    cameraDir.y = 0.0f;
    Vector3 cameraForward = cameraDir.Normalized();
    Vector3 cameraRight = Vector3(cameraForward.z, 0.0f, -cameraForward.x);

    const float moveSpeed = 50.0f;
    const float jumpForce = 5.0f;
    const float friction = 10.0f;

    Vector3 inputDirection = Vector3::Zero();
    if (Input::GetKeyPress(Input::KK_W))
    {
        inputDirection += cameraForward;
    }
    if (Input::GetKeyPress(Input::KK_S))
    {
        inputDirection -= cameraForward;
    }
    if (Input::GetKeyPress(Input::KK_A))
    {
        inputDirection -= cameraRight;
    }
    if (Input::GetKeyPress(Input::KK_D))
    {
        inputDirection += cameraRight;
    }

    if (Input::GetKeyPress(Input::KK_SHIFT))
    {
        inputDirection *= 2.0f;
    }

    Vector3 velocity = rigidbody->GetVelocity();
    if (inputDirection.LengthSqr() > 0.0f)
    {
        velocity += inputDirection.Normalized() * moveSpeed * Time::GetDeltaTime();
    }

    velocity.x -= velocity.x * friction * Time::GetDeltaTime();
    velocity.z -= velocity.z * friction * Time::GetDeltaTime();

    if (Input::GetKeyTrigger(Input::KK_SPACE) && m_isGrounded)
    {
        velocity.y += jumpForce;
        m_isGrounded = false;
        m_groundColliders.clear();

        if (m_jumpSE)
        {
            m_jumpSE->Play();
        }
    }

    rigidbody->SetVelocity(velocity);
    m_velocity = velocity;

    if (velocity.LengthSqr() > 0.01f)
    {
        const float targetRotationY = atan2f(velocity.x, velocity.z);
        const Vector3 currentRotation = transform()->GetRotation().QuatToEuler();
        transform()->SetRotation(Vector3(
            currentRotation.x,
            Math::LerpAngle(currentRotation.y, targetRotationY, 10.0f * Time::GetDeltaTime()),
            currentRotation.z));
    }

    if (Input::GetKeyTrigger(Input::KK_F))
    {
        GameObject* bullet = Manager::Instantiate(
            "Assets/Prefabs/Bullet.prefab",
            transform()->GetPosition() + transform()->GetForward() * 2.0f + Vector3(0.0f, 1.0f, 0.0f),
            Vector3::Zero(),
            Vector3::One());
        bullet->GetComponent<Bullet>()->SetVelocity(transform()->GetForward() * 20.0f);
    }

    auto enemies = Manager::GetObjectsByComponent<Enemy>();
    if (enemies.empty())
    {
        Manager::ChangeScene<ResultScene>();
    }
}

void Player::OnHitBegin(const HitInfo& hitInfo)
{
    if (!hitInfo.hit || hitInfo.isTrigger || hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

    // PhysicsはA側から見た押し戻し方向をnormalに入れるため、上向きなら足元の接触である。
    if (hitInfo.normal.y > 0.5f)
    {
        m_groundColliders.insert(hitInfo.colliderB);
        m_isGrounded = true;
    }
}

void Player::OnHit(const HitInfo& hitInfo)
{
    if (!hitInfo.hit || hitInfo.isTrigger || hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

    if (hitInfo.normal.y > 0.5f)
    {
        m_groundColliders.insert(hitInfo.colliderB);
        m_isGrounded = true;
    }
}

void Player::OnHitEnd(const HitInfo& hitInfo)
{
    if (hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

    m_groundColliders.erase(hitInfo.colliderB);
    m_isGrounded = !m_groundColliders.empty();
}
