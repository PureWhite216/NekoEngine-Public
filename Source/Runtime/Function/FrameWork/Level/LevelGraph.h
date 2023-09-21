#pragma once
#include "entt/entt.hpp"
#include "cereal/cereal.hpp"
namespace NekoEngine
{

    class LevelGraph
    {
    public:
        LevelGraph() = default;
        ~LevelGraph() = default;

        void Init(entt::registry& registry);
        void Update(entt::registry& registry);
        void UpdateTransform(entt::entity entity, entt::registry& registry);
        void DisableOnConstruct(bool diable, entt::registry& registry);
    };

    class Hierarchy
    {
    public:
        Hierarchy(entt::entity p);
        Hierarchy() = default;

        inline entt::entity Parent() const
        {
            return m_Parent;
        }
        inline entt::entity Next() const
        {
            return m_Next;
        }
        inline entt::entity Prev() const
        {
            return m_Prev;
        }
        inline entt::entity First() const
        {
            return m_First;
        }

        // Return true if rhs is an ancestor of rhs
        bool Compare(const entt::registry& registry, const entt::entity rhs) const;
        void Reset();

        // update hierarchy components when hierarchy component is added
        static void OnConstruct(entt::registry& registry, entt::entity entity);

        // update hierarchy components when hierarchy component is removed
        static void OnDestroy(entt::registry& registry, entt::entity entity);
        static void OnUpdate(entt::registry& registry, entt::entity entity);
        static void Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy);

        entt::entity m_Parent = entt::null;
        entt::entity m_First = entt::null;
        entt::entity m_Next = entt::null;
        entt::entity m_Prev = entt::null;

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("First", m_First), cereal::make_nvp("Next", m_Next), cereal::make_nvp("Previous", m_Prev), cereal::make_nvp("Parent", m_Parent));
        }
    };

} // NekoEngine

