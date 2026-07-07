#include "pch.h"

#include "System/renderer.h"
#include "System/GM_Time.h"

#include "Script/Ripple.h"
#include "Component/Primitive.h"
#include "Component/Transform.h"

void Ripple::OnStart()
{
    m_quad = GetComponent<Quad>();
    if(!m_quad)
    {
        m_quad = m_owner->AddComponent<Quad>(L"Assets/Texture/particle.png");
    }
}

void Ripple::Update()
{
    if (!m_quad) return;
    m_elapsedTime += Time::GetDeltaTime();
    float t = m_elapsedTime / m_lifeTime;
    MATERIAL mat = m_quad->GetMaterial();

    mat.Diffuse.w = 1.0f - t; // アルファ値を減少させる
    m_quad->SetMaterial(mat);
    float s = baseScale * (1.0f + t); // スケールを増加させる
    transform()->SetScale(Vector3(s, s, s));

    if( m_elapsedTime >= m_lifeTime)
    {
        m_owner->Destroy();
    }
}
