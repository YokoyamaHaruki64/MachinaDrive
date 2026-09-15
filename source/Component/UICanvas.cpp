#include "pch.h"

#include "Component/UICanvas.h"

#include <algorithm>

#include "Component/UITransform.h"
#include "Component/UIImage.h"
#include "System/Core/GameObject.h"
#include "System/Core/input.h"
#include "System/Core/manager.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Rendering/renderer.h"

namespace
{
    void UpdateChildLayouts(const TransformBase& parent, const UIRect& parentRect, float canvasScale)
    {
        for (TransformBase* child : parent.GetChildren())
        {
            UITransform* const uiTransform = dynamic_cast<UITransform*>(child);
            if (!uiTransform)
            {
                continue;
            }

            uiTransform->UpdateLayout(parentRect, canvasScale);
            UpdateChildLayouts(*uiTransform, uiTransform->GetPixelRect(), canvasScale);
        }
    }
}

UICanvas::UICanvas(GameObject* owner)
    : Component(owner)
{
}

void UICanvas::LateUpdate()
{
    const float width = static_cast<float>(Renderer::GetGameWidth());
    const float height = static_cast<float>(Renderer::GetGameHeight());
    if (width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    m_scale = std::min(width / ReferenceWidth, height / ReferenceHeight);
    const Vector2 canvasSize(ReferenceWidth * m_scale, ReferenceHeight * m_scale);
    m_pixelRect.min = Vector2((width - canvasSize.x) * 0.5f, (height - canvasSize.y) * 0.5f);
    m_pixelRect.max = m_pixelRect.min + canvasSize;

    if (UITransform* rootTransform = m_owner->GetUITransform())
    {
        UpdateChildLayouts(*rootTransform, m_pixelRect, m_scale);
    }
}

Vector2 UICanvas::GetPointerPosition() const
{
    const Vector2 pointerPixels = Input::GetMousePosition();
    if (m_scale <= 0.0f)
    {
        return Vector2::Zero();
    }

    return (pointerPixels - m_pixelRect.min) / m_scale;
}

bool UICanvas::IsPointerTarget(const UITransform& target) const
{
    const Vector2 pointer = Input::GetMousePosition();
    if (!target.ContainsPoint(pointer))
    {
        return false;
    }

    const UIImage* const targetImage = target.GetOwner()->GetComponent<UIImage>();
    if (!targetImage || !targetImage->IsRaycastTarget())
    {
        return false;
    }

    const int targetDrawLayer = targetImage->GetDrawLayer();

    const std::vector<std::unique_ptr<GameObject>>& gameObjects = Manager::GetActiveGameObjects();
    for (auto iterator = gameObjects.rbegin(); iterator != gameObjects.rend(); ++iterator)
    {
        GameObject* const gameObject = iterator->get();
        if (gameObject->IsDestroyed() || !gameObject->IsActive())
        {
            continue;
        }

        UITransform* const transform = gameObject->GetUITransform();
        UIImage* const image = gameObject->GetComponent<UIImage>();
        if (!transform || !image || !image->IsRaycastTarget() || transform->GetCanvas() != this || !transform->ContainsPoint(pointer))
        {
            continue;
        }

        // 入力の最前面も描画順に合わせる。同一レイヤーは生成順の後勝ちを維持する。
        if (image->GetDrawLayer() < targetDrawLayer)
        {
            continue;
        }

        if (image->GetDrawLayer() > targetDrawLayer)
        {
            return false;
        }

        return transform == &target;
    }

    return false;
}

bool UICanvas::GetOverlappingGameObjects(const UITransform& source, std::vector<GameObject*>& output) const
{
    output.clear();
    const std::vector<std::unique_ptr<GameObject>>& gameObjects = Manager::GetActiveGameObjects();
    for (auto iterator = gameObjects.rbegin(); iterator != gameObjects.rend(); ++iterator)
    {
        GameObject* const gameObject = iterator->get();
        if (gameObject == source.GetOwner() || gameObject->IsDestroyed() || !gameObject->IsActive())
        {
            continue;
        }

        UITransform* const uiTransform = gameObject->GetUITransform();
        if (!uiTransform || uiTransform->GetCanvas() != this || !source.Overlaps(*uiTransform))
        {
            continue;
        }

        output.push_back(gameObject);
    }
    return !output.empty();
}

void UICanvas::RegisterFields(FieldRegistry& fields)
{
}
