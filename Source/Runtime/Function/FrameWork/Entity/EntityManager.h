#pragma once

#include "Core.h"
#include "Entity.h"
#include "entt/entt.hpp"
namespace NekoEngine
{
    class Level;
    class Entity;

    template <typename... Component>
    class EntityView
    {
        using TView = entt::view<entt::get_t<Component...>>;
    public:
        EntityView(Level* level);

        Entity operator[](int i)
        {
            ASSERT(i >= Size(), "Index out of range on Entity View");
            return Entity(m_View[i], m_Level);
        }

        bool Empty() const { return m_View.empty(); }
        size_t Size() const { return m_View.size(); }
        Entity Front() { return Entity(m_View[0], m_Level); }

        class iterator
        {
        public:
            explicit iterator(EntityView<Component...>& view, size_t index = 0)
                    : view(view)
                    , nIndex(index)
            {
            }

            Entity operator*() const
            {
                return view[int(nIndex)];
            }
            iterator& operator++()
            {
                nIndex++;
                return *this;
            }
            iterator operator++(int)
            {
                return ++(*this);
            }
            bool operator!=(const iterator& rhs) const
            {
                return nIndex != rhs.nIndex;
            }

        private:
            size_t nIndex = 0;
            EntityView<Component...>& view;
        };

        Level* m_Level;
        TView m_View;

        iterator begin();
        iterator end();
    };

    template <typename... Component>
    EntityView<Component...>::EntityView(Level* level)
            : m_Level(level)
            , m_View(level->GetRegistry().view<Component...>())
    {
    }

    template <typename... Component>
    typename EntityView<Component...>::iterator EntityView<Component...>::begin()
    {
        return EntityView<Component...>::iterator(*this, 0);
    }

    template <typename... Component>
    typename EntityView<Component...>::iterator EntityView<Component...>::end()
    {
        return EntityView<Component...>::iterator(*this, Size());
    }

    class EntityManager
    {
    public:
        explicit EntityManager(Level* level)
                : m_Level(level)
        {
        }

        Entity Create();
        Entity Create(const std::string& name);

        template <typename... Components>
        auto GetEntitiesWithTypes()
        {
            return m_Registry.group<Components...>();
        }

        template <typename Component>
        EntityView<Component> GetEntitiesWithType()
        {
            return EntityView<Component>(m_Level);
        }

        template <typename R, typename T>
        void AddDependency()
        {
            m_Registry.template on_construct<R>().template connect<&entt::registry::get_or_emplace<T>>();
        }

        entt::registry& GetRegistry()
        {
            return m_Registry;
        }

        void Clear();

        Entity GetEntityByUUID(uint64_t id);

    private:
        Level* m_Level = nullptr;
        entt::registry m_Registry;
    };
}