#include "EntityManager.h"
#include "Level/Level.h"

namespace NekoEngine
{
    Entity EntityManager::Create()
    {
        auto e = m_Registry.create();
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Level);
    }

    Entity EntityManager::Create(const std::string &name)
    {
        auto e = m_Registry.create();
        m_Registry.emplace<NameComponent>(e, name);
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Level);
    }

    void EntityManager::Clear()
    {
        for(auto [entity] : m_Registry.storage<entt::entity>().each())
        {
            m_Registry.destroy(entity);
        }

        m_Registry.clear();
    }

    Entity EntityManager::GetEntityByUUID(uint64_t id)
    {
        auto view = m_Registry.view<IDComponent>();
        for(auto entity : view)
        {
            auto& idComponent = m_Registry.get<IDComponent>(entity);
            if(idComponent.ID == id)
                return {entity, m_Level};
        }

        LOG("Entity not found by ID");
        return Entity {};
    }
}