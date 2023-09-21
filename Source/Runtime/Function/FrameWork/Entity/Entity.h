#pragma once

#include "Core.h"
#include "UUID.h"
#include "Level/Level.h"
#include "Transform.h"

#include "entt/entt.hpp"


namespace NekoEngine
{
    class IDComponent
    {
    public:
        template <typename Archive>
        void save(Archive& archive) const
        {
            uint64_t uuid = (uint64_t)ID;
            archive(uuid);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            uint64_t uuid;
            archive(uuid);

            ID = UUID(uuid);
        }

    public:
        UUID ID;
    };

    class ActiveComponent
    {
    public:
        ActiveComponent()
        {
            active = true;
        }

        ActiveComponent(bool act)
        {
            active = act;
        }

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("Active", active));
        }
    public:
        bool active = true;
    };

    class NameComponent
    {
    public:
        std::string name;
    public:
        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("Name", name));
        }
    };

    class Entity
    {
    private:
        entt::entity m_EntityHandle;
        Level* m_Level = nullptr;

        friend class EntityManager;

    public:
        Entity() = default;

        Entity(entt::entity handle, Level* level)
            : m_EntityHandle(handle)
            , m_Level(level)
        {}
        ~Entity() = default;

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            return m_Level->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        T& GetOrAddComponent(Args&&... args)
        {
            return m_Level->GetRegistry().get_or_emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        void AddOrReplaceComponent(Args&&... args)
        {
            m_Level->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T>
        T& GetComponent()
        {
            return m_Level->GetRegistry().get<T>(m_EntityHandle);
        }

        template <typename T>
        T* TryGetComponent()
        {
            return m_Level->GetRegistry().try_get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent()
        {
            return m_Level->GetRegistry().all_of<T>(m_EntityHandle);
        }

        template <typename T>
        void RemoveComponent()
        {
            m_Level->GetRegistry().remove<T>(m_EntityHandle);
        }

        template <typename T>
        void TryRemoveComponent()
        {
            if(HasComponent<T>())
                RemoveComponent<T>();
        }

        bool Active()
        {
            bool active = true;
            if(HasComponent<ActiveComponent>())
                active = m_Level->GetRegistry().get<ActiveComponent>(m_EntityHandle).active;

            auto parent = GetParent();
            if(parent)
                active &= parent.Active();
            return active;
        }

        void SetActive(bool isActive)
        {
            
            GetOrAddComponent<ActiveComponent>().active = isActive;
        }

        Transform& GetTransform()
        {
            
            return m_Level->GetRegistry().get<Transform>(m_EntityHandle);
        }

        const Transform& GetTransform() const
        {
            
            return m_Level->GetRegistry().get<Transform>(m_EntityHandle);
        }

        uint64_t GetID()
        {
            
            return m_Level->GetRegistry().get<IDComponent>(m_EntityHandle).ID;
        }

        const std::string& GetName()
        {
            
            auto nameComponent = TryGetComponent<NameComponent>();

            if(nameComponent)
                return nameComponent->name;
            else
            {
                static std::string tempName = "Entity";
                return tempName;
            }
        }

        void SetParent(Entity entity)
        {
            
            bool acceptable         = false;
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent != nullptr)
            {
                acceptable = entity.m_EntityHandle != m_EntityHandle && (!entity.IsParent(*this)) && (hierarchyComponent->Parent() != m_EntityHandle);
            }
            else
                acceptable = entity.m_EntityHandle != m_EntityHandle;

            if(!acceptable)
            {
                LOG("Failed to parent entity!");
                return;
            }

            if(hierarchyComponent)
                Hierarchy::Reparent(m_EntityHandle, entity.m_EntityHandle, m_Level->GetRegistry(), *hierarchyComponent);
            else
            {
                m_Level->GetRegistry().emplace<Hierarchy>(m_EntityHandle, entity.m_EntityHandle);
            }
        }

        Entity GetParent()
        {
            
            auto hierarchyComp = TryGetComponent<Hierarchy>();
            if(hierarchyComp)
                return Entity(hierarchyComp->Parent(), m_Level);
            else
                return Entity(entt::null, nullptr);
        }

        std::vector<Entity> GetChildren()
        {
            
            std::vector<Entity> children;
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent)
            {
                entt::entity child = hierarchyComponent->First();
                while(child != entt::null && m_Level->GetRegistry().valid(child))
                {
                    children.emplace_back(child, m_Level);
                    hierarchyComponent = m_Level->GetRegistry().try_get<Hierarchy>(child);
                    if(hierarchyComponent)
                        child = hierarchyComponent->Next();
                }
            }

            return children;
        }

        void ClearChildren()
        {
            
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent)
            {
                hierarchyComponent->m_First = entt::null;
            }
        }

        bool IsParent(Entity potentialParent)
        {
            
            auto nodeHierarchyComponent = m_Level->GetRegistry().try_get<Hierarchy>(m_EntityHandle);
            if(nodeHierarchyComponent)
            {
                auto parent = nodeHierarchyComponent->Parent();
                while(parent != entt::null)
                {
                    if(parent == potentialParent.m_EntityHandle)
                    {
                        return true;
                    }
                    else
                    {
                        nodeHierarchyComponent = m_Level->GetRegistry().try_get<Hierarchy>(parent);
                        parent                 = nodeHierarchyComponent ? nodeHierarchyComponent->Parent() : entt::null;
                    }
                }
            }

            return false;
        }

        operator entt::entity() const
        {
            return m_EntityHandle;
        }

        operator uint32_t() const
        {
            return (uint32_t)m_EntityHandle;
        }

        operator bool() const
        {
            return m_EntityHandle != entt::null && m_Level;
        }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Level == other.m_Level;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        entt::entity GetHandle() const
        {
            return m_EntityHandle;
        }

        void Destroy()
        {
            m_Level->GetRegistry().destroy(m_EntityHandle);
        }

        bool Valid()
        {
            return m_Level->GetRegistry().valid(m_EntityHandle) && m_Level;
        }

        Level* GetLevel() const { return m_Level; }
    };
}