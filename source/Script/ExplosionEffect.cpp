#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/ExplosionEffect.h"

void ExplosionEffect::OnStart()
{
    // 初期のUV座標を設定w
    ModelRenderer* renderer = GetComponent<ModelRenderer>();
    // 透明描画パスに設定
    if (renderer != nullptr)
    {
        renderer->SetRenderPhase(RenderPhase::Transparent);

        // 最初のフレームのUV座標
        MaterialPropertyOverride& propertyOverride = renderer->GetMaterialPropertyOverride();
        propertyOverride.SetVector2("_UVScale", Vector2(1.0f / 4.0f, 1.0f / 4.0f));
        propertyOverride.SetVector2("_UVOffset", Vector2(0.0f, 0.0f));
    }
}

void ExplosionEffect::Update()
{
    m_elapsedTime += Time::GetDeltaTime(); // 経過時間を更新
    // エフェクトの寿命を超えたらゲームオブジェクトを削除
    if (m_elapsedTime >= LIFETIME)
    {
        m_owner->Destroy();
        return;
    }
    // スプライトシートのフレームを計算
    int currentFrame = static_cast<int>(m_elapsedTime / FRAME_DURATION);
    float width = 1.0f / 4.0f;
    float height = 1.0f / 4.0f;
    float offsetX = (currentFrame % 4) * width;
    float offsetY = (currentFrame / 4) * height;
    // UV座標を更新
    ModelRenderer* renderer = GetComponent<ModelRenderer>();
    if (renderer != nullptr)
    {
        MaterialPropertyOverride& propertyOverride = renderer->GetMaterialPropertyOverride();
        propertyOverride.SetVector2("_UVScale", Vector2(width, height));
        propertyOverride.SetVector2("_UVOffset", Vector2(offsetX, offsetY));
    }
}
