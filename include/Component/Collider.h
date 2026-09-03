#pragma once

#include "System/Core/component.h"
#include "math/Vector3.h"

class Rigidbody;

// Colliderは形状の共通設定と、所有ObjectのRigidbodyへの参照だけを持つ。
// 衝突判定と解決はPhysicsへ集約し、Collider自身はWorldを管理しない。
// 半径やサイズのような形状固有の値は派生Colliderが持つことで、判定側の
// 共通入口を保ちながら、後からCapsuleやMeshを追加できる構成にしている。
class Collider : public Component
{
protected:
    // 形状の中心はObjectの原点からのLocalオフセットとして保存する。
    // Colliderのcenterは形状の位置を指定する値なので、ObjectのScaleでは伸縮させず、
    // 回転と平行移動だけを適用してWorld座標へ変換する。
    Vector3 m_center{ Vector3::Zero() };
    bool m_isTrigger{ false };
    int m_layer{ 0 };

    explicit Collider(GameObject* owner);

    void RegisterFields(FieldRegistry& fields) override;

public:
    enum class ShapeType
    {
        Sphere,
        Box,
        Capsule,
    };

    Collider() = delete;
    ~Collider() override;

    void OnLoaded() override;
    void OnDestroy() override;

    virtual ShapeType GetShapeType() const = 0;

    const Vector3& GetCenter() const { return m_center; }
    void SetCenter(const Vector3& center);

    bool IsTrigger() const { return m_isTrigger; }
    void SetIsTrigger(bool isTrigger);

    int GetLayer() const { return m_layer; }
    void SetLayer(int layer);

    // 判定は常にObjectのTransformを正とする。Collider側で別の姿勢を持たないため、
    // 呼び出し側がTransformを更新した直後でも同じ情報からWorld座標を計算できる。
    Vector3 GetWorldCenter() const;
    Rigidbody* GetRigidbody() const;
};
