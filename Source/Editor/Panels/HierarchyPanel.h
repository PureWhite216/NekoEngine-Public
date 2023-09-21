#pragma once
#include "EditorPanel.h"
#include "ImGui/imgui.h"

namespace NekoEngine
{
    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel();
        ~HierarchyPanel() = default;

        void DrawNode(entt::entity node, entt::registry& registry);
        void OnImGui() override;

        void DestroyEntity(entt::entity entity, entt::registry& registry);
        bool IsParentOfEntity(entt::entity entity, entt::entity child, entt::registry& registry);

    private:
        ImGuiTextFilter m_HierarchyFilter;
        entt::entity m_DoubleClicked;
        entt::entity m_HadRecentDroppedEntity;
        entt::entity m_CurrentPrevious;
        bool m_SelectUp;
        bool m_SelectDown;
    };

} // NekoEngine

