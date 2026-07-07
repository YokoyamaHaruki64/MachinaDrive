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
    Curve<float> m_widthCurve =
        Curve<float>({
            { 1.0f, 0.0f },
            { 0.5f, 1.0f }
        });
    Curve<Color> m_colorCurve =
        Curve<Color>({
            { Color::White(), 0.0f },
            { Color(1.0f, 1.0f, 1.0f, 0.0f), 1.0f }
        });

    float m_trailLifetime = 1.0f; // トレイルの寿命（秒）
    float m_minSegmentDistance = 0.1f; // 新しいセグメントを追加する最小距離
    Vector3 m_offset = Vector3::Zero(); // トレイルのオフセット
    TrailFaceMode m_faceMode = TrailFaceMode::CameraFacing; // トレイルの向きモード
    Vector3 m_fixedDirection = Vector3::Up(); // 固定方向モードの向き

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
    void SetWidthCurve(const Curve<float>& curve) { m_widthCurve = curve; }
    void SetColorCurve(const Curve<Color>& curve) { m_colorCurve = curve; }
    virtual void Draw() override;

    void SetFaceMode(TrailFaceMode mode) { m_faceMode = mode; }
    void SetFixedDirection(const Vector3& direction) { m_fixedDirection = direction; }
    void SetMinSegmentDistance(float distance) { m_minSegmentDistance = distance; }
    void SetTrailLifetime(float lifetime) { m_trailLifetime = lifetime; }
};
