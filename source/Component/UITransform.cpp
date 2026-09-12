#include "pch.h"

#include "Component/UITransform.h"

#include "Component/UICanvas.h"
#include "System/Core/GameObject.h"
#include "System/Editor/FieldRegistry.h"

namespace
{
    constexpr float ReferenceAspectRatio = 1920.0f / 1080.0f;

    Vector2 ToNormalizedPosition(UIPositionPreset preset)
    {
        switch (preset)
        {
        case UIPositionPreset::LeftUp: return Vector2(0.0f, 0.0f);
        case UIPositionPreset::CenterUp: return Vector2(0.5f, 0.0f);
        case UIPositionPreset::RightUp: return Vector2(1.0f, 0.0f);
        case UIPositionPreset::LeftCenter: return Vector2(0.0f, 0.5f);
        case UIPositionPreset::Center: return Vector2(0.5f, 0.5f);
        case UIPositionPreset::RightCenter: return Vector2(1.0f, 0.5f);
        case UIPositionPreset::LeftBottom: return Vector2(0.0f, 1.0f);
        case UIPositionPreset::CenterBottom: return Vector2(0.5f, 1.0f);
        case UIPositionPreset::RightBottom: return Vector2(1.0f, 1.0f);
        default: return Vector2(0.5f, 0.5f);
        }
    }

    UIPositionPreset ToPositionPreset(const Vector2& value)
    {
        const int column = value.x < 0.25f ? 0 : value.x > 0.75f ? 2 : 1;
        const int row = value.y < 0.25f ? 0 : value.y > 0.75f ? 2 : 1;
        return static_cast<UIPositionPreset>(row * 3 + column);
    }

    XMMATRIX CreateWorldMatrix(const Vector2& size, const Vector2& pivot, const Vector2& pivotPosition, float rotation)
    {
        const Vector2 localPivotOffset(
            (0.5f - pivot.x) * size.x,
            (0.5f - pivot.y) * size.y);
        return XMMatrixScaling(size.x, size.y, 1.0f)
            * XMMatrixTranslation(localPivotOffset.x, localPivotOffset.y, 0.0f)
            * XMMatrixRotationZ(rotation)
            * XMMatrixTranslation(pivotPosition.x, pivotPosition.y, 0.0f);
    }

    bool AreQuadsOverlapping(const std::array<Vector2, 4>& first, const std::array<Vector2, 4>& second)
    {
        const auto isSeparatedOnEdgeNormals = [](const std::array<Vector2, 4>& source, const std::array<Vector2, 4>& target)
        {
            for (size_t index = 0; index < source.size(); ++index)
            {
                const Vector2 edge = source[(index + 1) % source.size()] - source[index];
                const Vector2 axis(-edge.y, edge.x);
                float sourceMinimum = Vector2::Dot(source[0], axis);
                float sourceMaximum = sourceMinimum;
                float targetMinimum = Vector2::Dot(target[0], axis);
                float targetMaximum = targetMinimum;

                for (size_t cornerIndex = 1; cornerIndex < source.size(); ++cornerIndex)
                {
                    const float sourceProjection = Vector2::Dot(source[cornerIndex], axis);
                    sourceMinimum = (std::min)(sourceMinimum, sourceProjection);
                    sourceMaximum = (std::max)(sourceMaximum, sourceProjection);
                    const float targetProjection = Vector2::Dot(target[cornerIndex], axis);
                    targetMinimum = (std::min)(targetMinimum, targetProjection);
                    targetMaximum = (std::max)(targetMaximum, targetProjection);
                }

                if (sourceMaximum < targetMinimum || targetMaximum < sourceMinimum)
                {
                    return true;
                }
            }
            return false;
        };

        return !isSeparatedOnEdgeNormals(first, second)
            && !isSeparatedOnEdgeNormals(second, first);
    }
}

UITransform::UITransform(GameObject* owner)
    : TransformBase(owner)
{
}

void UITransform::SetAnchorPresetValue(const Vector2& value)
{
    m_anchor = value;
}

float UITransform::GetStretchAspectRatio() const
{
    if (m_aspectRatio > 0.0f)
    {
        return m_aspectRatio / (m_parentAspectRatio > 0.0f ? m_parentAspectRatio : ReferenceAspectRatio);
    }

    return m_stretch.y > 0.0f ? m_stretch.x / m_stretch.y : 1.0f;
}

void UITransform::SetStretch(const Vector2& value)
{
    const Vector2 previousStretch = m_stretch;
    const float stretchAspectRatio = GetStretchAspectRatio();
    m_stretch = value;
    if (!m_lockAspectRatio)
    {
        return;
    }

    if (stretchAspectRatio <= 0.0f)
    {
        return;
    }

    const float deltaX = std::abs(value.x - previousStretch.x);
    const float deltaY = std::abs(value.y - previousStretch.y);
    if (deltaX >= deltaY)
    {
        m_stretch.y = value.x / stretchAspectRatio;
    }
    else
    {
        m_stretch.x = value.y * stretchAspectRatio;
    }
}

void UITransform::SetAnchor(UIPositionPreset value)
{
    SetAnchorPresetValue(ToNormalizedPosition(value));
}

void UITransform::SetPivot(UIPositionPreset value)
{
    SetPivotPresetValue(ToNormalizedPosition(value));
}

UIPositionPreset UITransform::GetAnchor() const
{
    return ToPositionPreset(m_anchor);
}

UIPositionPreset UITransform::GetPivot() const
{
    return ToPositionPreset(m_pivot);
}

void UITransform::OnLoaded()
{
    m_canvas = ResolveCanvas();
}

void UITransform::LateUpdate()
{
    // CanvasがないUI Objectは描画位置を導出できないため、通常Transformのまま保持する。
    m_canvas = ResolveCanvas();
}

UICanvas* UITransform::ResolveCanvas() const
{
    for (const TransformBase* transform = this; transform; transform = transform->GetParent())
    {
        GameObject* const gameObject = transform->GetOwner();
        if (UICanvas* canvas = gameObject->GetComponent<UICanvas>())
        {
            return canvas;
        }
    }

    return nullptr;
}

UICanvas* UITransform::GetCanvas() const
{
    return ResolveCanvas();
}

void UITransform::UpdateLayout(const UIRect& parentRect, float canvasScale)
{
    m_canvasScale = canvasScale;
    const Vector2 parentSize = parentRect.GetSize();
    m_parentAspectRatio = parentSize.y > 0.0f ? parentSize.x / parentSize.y : ReferenceAspectRatio;

    const Vector2 size = Vector2::Multiply(parentSize, Vector2::Multiply(m_stretch, m_uiScale));
    const Vector2 anchorPosition = parentRect.min + Vector2::Multiply(parentSize, m_anchor);
    const Vector2 position = anchorPosition + Vector2::Multiply(parentSize, m_pos) + m_offset * canvasScale;
    m_pixelRect.min = position - Vector2::Multiply(m_pivot, size);
    m_pixelRect.max = m_pixelRect.min + size;

}

XMMATRIX UITransform::GetWorldMatrix() const
{
    const Vector2 size = m_pixelRect.GetSize();
    const Vector2 pivotPosition = m_pixelRect.min + Vector2::Multiply(m_pivot, size);
    const UITransform* const parent = dynamic_cast<const UITransform*>(GetParent());
    if (!parent || parent->GetOwner()->GetComponent<UICanvas>())
    {
        return CreateWorldMatrix(size, m_pivot, pivotPosition, m_uiRotation);
    }

    const Vector2 parentSize = parent->m_pixelRect.GetSize();
    if (std::abs(parentSize.x) <= 1.0e-6f || std::abs(parentSize.y) <= 1.0e-6f)
    {
        return CreateWorldMatrix(size, m_pivot, pivotPosition, m_uiRotation);
    }

    // 子は親Rect内の正規化座標で組み、親のScaleとRotationを一度だけ継承する。
    const Vector2 localSize(size.x / parentSize.x, size.y / parentSize.y);
    const Vector2 localPivotPosition(
        (pivotPosition.x - parent->m_pixelRect.min.x) / parentSize.x - 0.5f,
        (pivotPosition.y - parent->m_pixelRect.min.y) / parentSize.y - 0.5f);
    return CreateWorldMatrix(localSize, m_pivot, localPivotPosition, m_uiRotation)
        * parent->GetWorldMatrix();
}

std::array<Vector2, 4> UITransform::GetWorldCorners() const
{
    static constexpr std::array<Vector2, 4> localCorners{
        Vector2(-0.5f, -0.5f), Vector2(0.5f, -0.5f),
        Vector2(0.5f, 0.5f), Vector2(-0.5f, 0.5f),
    };

    const XMMATRIX worldMatrix = GetWorldMatrix();
    std::array<Vector2, 4> corners;
    for (size_t index = 0; index < corners.size(); ++index)
    {
        const XMVECTOR transformed = XMVector3TransformCoord(
            XMVectorSet(localCorners[index].x, localCorners[index].y, 0.0f, 1.0f),
            worldMatrix);
        corners[index] = Vector2(XMVectorGetX(transformed), XMVectorGetY(transformed));
    }
    return corners;
}

bool UITransform::ContainsPoint(const Vector2& point) const
{
    const XMMATRIX worldMatrix = GetWorldMatrix();
    const XMVECTOR determinant = XMMatrixDeterminant(worldMatrix);
    if (std::abs(XMVectorGetX(determinant)) <= 1.0e-6f)
    {
        return false;
    }

    const XMVECTOR local = XMVector3TransformCoord(
        XMVectorSet(point.x, point.y, 0.0f, 1.0f),
        XMMatrixInverse(nullptr, worldMatrix));
    return std::abs(XMVectorGetX(local)) <= 0.5f
        && std::abs(XMVectorGetY(local)) <= 0.5f;
}

bool UITransform::Overlaps(const UITransform& other) const
{
    return AreQuadsOverlapping(GetWorldCorners(), other.GetWorldCorners());
}

void UITransform::SetUIScale(const Vector2& value)
{
    const Vector2 previousScale = m_uiScale;
    m_uiScale = value;
    if (!m_lockAspectRatio || previousScale.x <= 0.0f || previousScale.y <= 0.0f)
    {
        return;
    }

    const float previousAspectRatio = previousScale.x / previousScale.y;
    const float deltaX = std::abs(value.x - previousScale.x);
    const float deltaY = std::abs(value.y - previousScale.y);
    if (deltaX >= deltaY)
    {
        m_uiScale.y = value.x / previousAspectRatio;
    }
    else
    {
        m_uiScale.x = value.y * previousAspectRatio;
    }
}

void UITransform::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_ACCESSOR_TYPE(Vector2, "anchorPos", m_anchor, SetAnchorPresetValue(value), All, InspectorFieldType::PositionPreset, false);
    GM_FIELD_ACCESSOR_TYPE(Vector2, "pivotPos", m_pivot, SetPivotPresetValue(value), All, InspectorFieldType::PositionPreset, false);
    GM_FIELD_ACCESSOR(Vector2, "stretch", GetStretch(), SetStretch(value), All);
    GM_FIELD_ACCESSOR(Vector2, "pos", GetPos(), SetPos(value), All);
    GM_FIELD_ACCESSOR(float, "rotation", GetUIRotation(), SetUIRotation(value), All);
    GM_FIELD_ACCESSOR(bool, "lockAspectRatio", IsAspectRatioLocked(), SetLockAspectRatio(value), All);

    // ScaleとOffsetはScript専用のためInspectorには出さない。
    GM_FIELD_TYPE(m_anchor, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_pivot, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_stretch, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_pos, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_offset, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_uiRotation, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_uiScale, InspectorFieldType::Hidden);
    GM_FIELD_TYPE(m_aspectRatio, InspectorFieldType::Hidden);
}
