#include "Score.h"

#include "Component/ComponentInc.h"


void Score::OnStart()
{
    for(int i = 0; i < MAX_DIGITS; ++i)
    {
        const float OFFSET = 60.0f * i;
        Vector3 pos = Vector3(
            transform()->GetPosition().x + OFFSET,
            transform()->GetPosition().y,
            transform()->GetPosition().z);
        GameObject* digitObj = Manager::CreateEmpty(pos);
        digitObj->AddComponent<SpriteRenderer>(
            L"Assets\\Textures\\number.png")->SetUVSize(1.0f / 5.0f, 1.0f / 5.0f);

        digitObj->GetTransform()->SetScale(Vector3(100.0f, 100.0f, 1.0f));

        m_scoreObjects.push_back(digitObj);
    }
}

void Score::Update()
{
    int score = m_score;
    for (int i = MAX_DIGITS - 1; i >= 0; --i)
    {
        int digit = score % 10;
        score /= 10;
        SpriteRenderer* renderer = m_scoreObjects[i]->GetComponent<SpriteRenderer>();
        if(renderer)
        {
            float uvOffsetX = digit % 5 * 0.2f;
            float uvOffsetY = digit / 5 * 0.2f;
            renderer->SetUVOffset(uvOffsetX, uvOffsetY);
        }
    }
}
