#include "pch.h"

#include "System/ComponentRegistry.h"

#include "System/GameObject.h"

const std::vector<ComponentRegistration>& ComponentRegistry::GetRegistrations()
{
    return GetMutableRegistrations();
}

const ComponentRegistration* ComponentRegistry::Find(std::type_index type)
{
    for (const ComponentRegistration& registration : GetMutableRegistrations())
    {
        if (registration.type == type)
        {
            return &registration;
        }
    }

    return nullptr;
}

bool ComponentRegistry::CanAdd(const ComponentRegistration& registration, const GameObject& gameObject)
{
    if (!registration.factory || registration.allowMultiple)
    {
        return registration.factory != nullptr;
    }

    for (const Component* component : gameObject.GetComponents())
    {
        if (std::type_index(typeid(*component)) == registration.type)
        {
            return false;
        }
    }

    return true;
}

Component* ComponentRegistry::Add(const ComponentRegistration& registration, GameObject& gameObject)
{
    if (!CanAdd(registration, gameObject))
    {
        return nullptr;
    }

    return registration.factory(gameObject);
}

void ComponentRegistry::RegisterInternal(ComponentRegistration registration)
{
    std::vector<ComponentRegistration>& registrations = GetMutableRegistrations();
    for (ComponentRegistration& registered : registrations)
    {
        if (registered.type == registration.type)
        {
            registered = std::move(registration);
            return;
        }
    }

    registrations.emplace_back(std::move(registration));
}

std::vector<ComponentRegistration>& ComponentRegistry::GetMutableRegistrations()
{
    static std::vector<ComponentRegistration> registrations;
    return registrations;
}
