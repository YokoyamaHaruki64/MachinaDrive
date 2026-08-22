#include "pch.h"

#include "Component/ComponentInc.h"


#include "Script/Ripple.h"
#include "Common/Random.h"
#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/TextureManager.h"

void Ripple::OnStart()
{
    int colorIndex = Random::Range(0, COLOR_PALLET_SIZE - 1);
    Color c = m_colorPallet[colorIndex];
    m_colorCurve = Curve<Color>(m_colorPallet[colorIndex], Color(c.r, c.g, c.b, 0.0f)); // 最後のカラーは透明にする

    m_renderer = GetComponent<ModelRenderer>();
    if (!m_renderer)
    {
        m_renderer = m_owner->AddComponent<ModelRenderer>(
            ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad));
        GraphicsShader* shader = ShaderManager::LoadShaderFromFile("Assets/Shaders/UnlitAdditive.shader");
        if (Material* material = shader ? MaterialManager::Create("Ripple.Fallback", *shader) : nullptr)
        {
            material->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Local\\Textures\\Ripple.png"));
            m_renderer->SetMaterial(0, material);
        }
    }

    if (Material* material = m_renderer->GetMaterial())
        material->SetColor("_BaseColor", c);

    transform()->SetScale(Vector3(baseScale, baseScale, baseScale));
}

void Ripple::Update()
{
    if (!m_renderer) return;
    m_elapsedTime += Time::GetDeltaTime();
    float t = m_elapsedTime / m_lifeTime;
    Color c = m_colorCurve.Evaluate(t); // カラーを更新
    if (Material* material = m_renderer->GetMaterial())
        material->SetColor("_BaseColor", c);
    float s = baseScale * (1.5f + t); // スケールを増加させる
    transform()->SetScale(Vector3(s, s, s));

    if( m_elapsedTime >= m_lifeTime)
    {
        m_owner->Destroy();
    }
}
