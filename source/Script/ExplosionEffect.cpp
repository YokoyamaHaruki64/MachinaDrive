#include "pch.h"

#include "Script/ExplosionEffect.h"

#include "System/GM_Time.h"
#include "Component/Primitive.h"


void ExplosionEffect::OnStart()
{
    // 初期のUV座標を設定w
    Quad* renderer = GetComponent<Quad>();
    // 透明描画パスに設定
    if (renderer != nullptr)
    {
        renderer->SetRenderPass(RenderPass::Transparent);
        // 最初のフレームのUV座標
        renderer->SetUVRect(0.0f, 0.0f, 1.0f / 4.0f, 1.0f / 4.0f); 
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
    Quad* renderer = GetComponent<Quad>();
    if (renderer != nullptr)
    {
        renderer->SetUVRect(offsetX, offsetY, width, height);
    }
}
