#pragma once

#include <array>

#include "System/Core/main.h"
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"
#include "Component/TransformBase.h"
#include "math/Matrix4x4.h"
#include "System/UI/UIRect.h"

class UICanvas;

// UIの基準位置。AnchorとPivotで同じ9点プリセットを使う。
enum class UIPositionPreset
{
    LeftUp,
    CenterUp,
    RightUp,
    LeftCenter,
    Center,
    RightCenter,
    LeftBottom,
    CenterBottom,
    RightBottom,
};

// Canvas内の比率Layoutと演出用Canvas単位を、描画用Transformへ導出する。
class UITransform final : public TransformBase
{
    REGISTER_COMPONENT_OPTIONS(UITransform, "UI Transform", false, false);

    Vector2 m_anchor{ 0.5f, 0.5f };
    Vector2 m_pivot{ 0.5f, 0.5f };
    Vector2 m_stretch{ 100.0f / 1920.0f, 100.0f / 1080.0f };
    Vector2 m_pos{};
    Vector2 m_offset{};
    float m_uiRotation{};
    Vector2 m_uiScale{ 1.0f, 1.0f };
    bool m_lockAspectRatio = false;
    float m_aspectRatio{};
    UIRect m_pixelRect{};
    float m_canvasScale{ 1.0f };
    float m_parentAspectRatio{ 1920.0f / 1080.0f };
    UICanvas* m_canvas{};

    UICanvas* ResolveCanvas() const;
    void SetAnchorPresetValue(const Vector2& value);
    void SetPivotPresetValue(const Vector2& value) { m_pivot = value; }
    float GetStretchAspectRatio() const;

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UITransform() = delete;
    explicit UITransform(GameObject* owner);

    void OnLoaded() override;
    void LateUpdate() override;

    void SetStretch(const Vector2& value);
    void SetPos(const Vector2& value) { m_pos = value; }
    void SetOffset(const Vector2& value) { m_offset = value; }
    void AddOffset(const Vector2& value) { m_offset += value; }
    void SetUIRotation(float value) { m_uiRotation = value; }
    void SetUIScale(const Vector2& value);
    void AddScale(const Vector2& value) { SetUIScale(m_uiScale + value); }
    void SetAnchor(UIPositionPreset value);
    void SetPivot(UIPositionPreset value);
    void SetAspectRatio(float value) { if (value > 0.0f) m_aspectRatio = value; }
    void SetLockAspectRatio(bool value) { m_lockAspectRatio = value; }

    const Vector2& GetStretch() const { return m_stretch; }
    const Vector2& GetPos() const { return m_pos; }
    const Vector2& GetOffset() const { return m_offset; }
    float GetUIRotation() const { return m_uiRotation; }
    const Vector2& GetUIScale() const { return m_uiScale; }
    float GetAspectRatio() const { return m_aspectRatio; }
    bool IsAspectRatioLocked() const { return m_lockAspectRatio; }
    UIPositionPreset GetAnchor() const;
    UIPositionPreset GetPivot() const;

    UICanvas* GetCanvas() const;
    const UIRect& GetPixelRect() const { return m_pixelRect; }
    Vector2 GetCenterPixels() const { return m_pixelRect.GetCenter(); }
    Vector2 GetSizePixels() const { return m_pixelRect.GetSize(); }
    XMMATRIX GetWorldMatrix() const;
    std::array<Vector2, 4> GetWorldCorners() const;
    bool ContainsPoint(const Vector2& point) const;
    bool Overlaps(const UITransform& other) const;

    void UpdateLayout(const UIRect& parentRect, float canvasScale);
};
