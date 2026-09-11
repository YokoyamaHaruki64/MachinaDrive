#pragma once

#include <cstddef>
#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>
#include <wrl/client.h>

#include "math/Quaternion.h"
#include "math/Vector3.h"

class Collider;
class BoxCollider;
class CapsuleCollider;
class GraphicsShader;
class SphereCollider;
class Transform;

// Debug用の線描画を一元化するクラス。
// Colliderは生成時に登録され、描画時に現在のTransformからWire形状を再生成する。
// 形状ごとのRendererや個別VertexBufferは持たず、全Colliderを一括描画する。
class DebugDrawer final
{
private:
    static constexpr const char* DEBUG_COLLIDER_SHADER_PATH = "Assets/Shaders/DebugCollider.shader";
    static constexpr int CIRCLE_SEGMENT_COUNT = 32;

    // Positionだけを持つため、既存のPositionストリームと同じ12byteの頂点になる。
    // 色は全Collider共通のShader定数とし、Colorストリーム用の別バッファを不要にする。
    struct DebugVertex
    {
        Vector3 position;
    };

    static std::vector<Collider*> s_colliders;
    static std::vector<DebugVertex> s_vertices;
    static std::vector<DebugVertex> s_unitSphereVertices;
    static std::vector<DebugVertex> s_unitBoxVertices;
    static std::vector<DebugVertex> s_unitLowerHemisphereVertices;
    static std::vector<DebugVertex> s_unitUpperHemisphereVertices;
    static std::vector<DebugVertex> s_unitCylinderVertices;
    static Microsoft::WRL::ComPtr<ID3D11Buffer> s_vertexBuffer;
    static std::size_t s_vertexBufferCapacity;
    static GraphicsShader* s_shader;
    static bool s_enabled;

    static float GetMaximumAbsScale(const Transform* transform);
    static Quaternion GetWorldRotation(const Collider& collider);
    static Vector3 GetWorldScale(const Collider& collider);
    static DirectX::XMMATRIX CreateTransformMatrix(
        const Vector3& scale,
        const Quaternion& rotation,
        const Vector3& translation);

    static void AddLine(
        std::vector<DebugVertex>& vertices,
        const Vector3& start,
        const Vector3& end);
    static void AddCircle(
        std::vector<DebugVertex>& vertices,
        const Vector3& center,
        const Vector3& basisA,
        const Vector3& basisB,
        float radius);
    static void BuildUnitHemisphere(
        std::vector<DebugVertex>& vertices,
        const Vector3& capAxis);
    static void BuildUnitShapes();
    static void AppendTransformed(
        const std::vector<DebugVertex>& vertices,
        DirectX::FXMMATRIX transform);
    static void AppendSphere(const SphereCollider& collider);
    static void AppendBox(const BoxCollider& collider);
    static void AppendCapsule(const CapsuleCollider& collider);
    static void AppendCollider(const Collider& collider);
    static bool EnsureVertexBuffer(std::size_t vertexCount);

public:
    static void Initialize();
    static void Uninitialize();

    static void RegisterCollider(Collider* collider);
    static void UnregisterCollider(Collider* collider);

    static bool IsEnabled() { return s_enabled; }
    static void SetEnabled(bool enabled) { s_enabled = enabled; }

    // 現在登録されている有効なColliderから頂点を生成し、LineListで一度だけ描画する。
    static void Draw();
};
