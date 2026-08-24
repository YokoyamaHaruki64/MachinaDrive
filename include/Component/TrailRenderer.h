#pragma once

#include "Common/Color.h"
#include "Graphics/Material.h"
#include "Graphics/ModelAsset.h"
#include "System/RendererBase.h"
#include "System/ComponentRegistry.h"
#include "math/Curve.h"
#include "math/Vector3.h"

#include <deque>

class TrailRenderer : public RendererBase
{
    REGISTER_COMPONENT(TrailRenderer, "Trail Renderer");

public:
    enum TrailFaceMode
    {
        CameraFacing,
        FixedDirection
    };

private:
    static constexpr int MAX_SEGMENTS = 1000;

    struct TrailSegment
    {
        Vector3 position;
        float elapsedTime;

        TrailSegment(const Vector3& pos)
            : position(pos), elapsedTime(0.0f) {}
    };

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;

    Material* m_materialInstance{};
    TextureHandle m_texture{};
    std::string m_texturePath;
    std::string m_materialPath{ "Assets/Materials/UnlitAdditive.material" };

    std::deque<TrailSegment> m_segments;
    Curve<float> m_widthCurve = Curve<float>(1.0f, 0.0f);
    CurveColor m_colorCurve = CurveColor(Color::White(), Color(1.0f, 1.0f, 1.0f, 0.0f));

    Vector3 m_offset = Vector3::Zero();
    Vector3 m_fixedDirection = Vector3::Up();
    TrailFaceMode m_faceMode = TrailFaceMode::CameraFacing;
    float m_trailLifetime = 1.0f;
    float m_minSegmentDistance = 0.1f;
    float m_width = 1.0f;

    void UpdateSegments();
    void CreateVertexStreams();

public:
    TrailRenderer() = delete;
    explicit TrailRenderer(GameObject* owner);
    explicit TrailRenderer(GameObject* owner, const wchar_t* textureFile);
    virtual ~TrailRenderer() {}
    void OnStart() override;
    void Draw() override;
    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

    const Curve<float>& GetWidthCurve() const { return m_widthCurve; }
    const CurveColor& GetColorCurve() const { return m_colorCurve; }
    void SetWidthCurve(const Curve<float>& curve) { m_widthCurve = curve; }
    void SetColorCurve(const CurveColor& curve) { m_colorCurve = curve; }

    TrailFaceMode GetFaceMode() const { return m_faceMode; }
    const Vector3& GetFixedDirection() const { return m_fixedDirection; }
    const Vector3& GetOffset() const { return m_offset; }
    float GetMinSegmentDistance() const { return m_minSegmentDistance; }
    float GetTrailLifetime() const { return m_trailLifetime; }
    float GetWidth() const { return m_width; }

    void SetFaceMode(TrailFaceMode mode) { m_faceMode = mode; }
    void SetFixedDirection(const Vector3& direction) { m_fixedDirection = direction; }
    void SetOffset(const Vector3& offset) { m_offset = offset; }
    void SetMinSegmentDistance(float distance) { m_minSegmentDistance = distance; }
    void SetTrailLifetime(float lifetime) { m_trailLifetime = lifetime; }
    void SetWidth(float width) { m_width = width; }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
