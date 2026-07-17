#pragma once

#include "System/main.h"
#include "System/RendererBase.h"
#include "math/Vector3.h"
#include "math/Curve.h"
#include "Common/Color.h"

#include <deque>

class TrailRenderer : public RendererBase
{
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

    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11ShaderResourceView> m_texture;

    std::deque<TrailSegment> m_segments;
    Curve<float> m_widthCurve = Curve<float>(1.0f, 0.0f);
    CurveColor m_colorCurve =
        CurveColor(Color::White(), Color(1.0f, 1.0f, 1.0f, 0.0f));

    Vector3 m_offset = Vector3::Zero(); // トレイルのオフセット
    Vector3 m_fixedDirection = Vector3::Up(); // 固定方向モードの向き
    TrailFaceMode m_faceMode = TrailFaceMode::CameraFacing; // トレイルの向きモード
    float m_trailLifetime = 1.0f; // トレイルの寿命（秒）
    float m_minSegmentDistance = 0.1f; // 新しいセグメントを追加する最小距離
    float m_width = 1.0f; // トレイルの幅

    void UpdateSegments();
public:
    TrailRenderer() = delete;
    explicit TrailRenderer(
        GameObject* owner,
        const wchar_t* textureFile,
        const char* vertexShaderFile = "BuildedShader/unlitTextureVS.cso",
        const char* pixelShaderFile = "BuildedShader/unlitTexturePS.cso"
    );
    virtual ~TrailRenderer() {}
    virtual void Draw() override;

    const Curve<float>& GetWidthCurve() const { return m_widthCurve; }
    const CurveColor& GetColorCurve() const { return m_colorCurve; }
    void SetWidthCurve(const Curve<float>& curve) { m_widthCurve = curve; }
    void SetColorCurve(const CurveColor& curve) { m_colorCurve = curve; }

    const TrailFaceMode GetFaceMode() const { return m_faceMode; }
    const Vector3& GetFixedDirection() const { return m_fixedDirection; }
    const Vector3& GetOffset() const { return m_offset; }
    const float GetMinSegmentDistance() const { return m_minSegmentDistance; }
    const float GetTrailLifetime() const { return m_trailLifetime; }
    const float GetWidth() const { return m_width; }

    void SetFaceMode(TrailFaceMode mode) { m_faceMode = mode; }
    void SetFixedDirection(const Vector3& direction) { m_fixedDirection = direction; }
    void SetOffset(const Vector3& offset) { m_offset = offset; }
    void SetMinSegmentDistance(float distance) { m_minSegmentDistance = distance; }
    void SetTrailLifetime(float lifetime) { m_trailLifetime = lifetime; }
    void SetWidth(float width) { m_width = width; }
};
