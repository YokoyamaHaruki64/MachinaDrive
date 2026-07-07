#include "pch.h"

#include "System/renderer.h"
#include "System/GM_Time.h"

#include "Script/Ripple.h"
#include "Component/Primitive.h"
#include "Component/Transform.h"
#include "Common/Random.h"

void Ripple::OnStart()
{
    int colorIndex = Random::Range(0, COLOR_PALLET_SIZE - 1);
    Color c = m_colorPallet[colorIndex];
    m_colorCurve = Curve<Color>(m_colorPallet[colorIndex], Color(c.r, c.g, c.b, 0.0f)); // 最後のカラーは透明にする

    m_quad = GetComponent<Quad>();
    if(!m_quad)
    {
        m_quad = m_owner->AddComponentForCreate<Quad>(L"Assets/Texture/Ripple.png");
    }
    MATERIAL mat = m_quad->GetMaterial();
    mat.Diffuse = XMFLOAT4(c.r, c.g, c.b, c.a);
    m_quad->SetMaterial(mat);
    transform()->SetScale(Vector3(baseScale, baseScale, baseScale));
}

void Ripple::Update()
{
    if (!m_quad) return;
    m_elapsedTime += Time::GetDeltaTime();
    float t = m_elapsedTime / m_lifeTime;
    MATERIAL mat = m_quad->GetMaterial();

    Color c = m_colorCurve.Evaluate(t); // カラーを更新
    mat.Diffuse = XMFLOAT4(c.r, c.g, c.b, c.a);
    m_quad->SetMaterial(mat);
    float s = baseScale * (1.5f + t); // スケールを増加させる
    transform()->SetScale(Vector3(s, s, s));

    if( m_elapsedTime >= m_lifeTime)
    {
        m_owner->Destroy();
    }
}
