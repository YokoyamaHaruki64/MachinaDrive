#pragma once

template <typename T>
T* Component::GetComponent() const
{
    return m_owner ? m_owner->GetComponent<T>() : nullptr;
}
