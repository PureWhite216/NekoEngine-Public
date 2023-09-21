#include "GUI/ImGuiUtility.h"
#include "HierarchyPanel.h"
#include "Editor.h"
#include "Renderable/Environment.h"
#include "Entity/Entity.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include "Script/LuaScriptComponent.h"
#include "Component/PrefabComponent.h"
#include "Component/RigidBody3DComponent.h"
#include "Component/ConstraintComponent.h"

#define INPUT_BUF_SIZE 128

namespace NekoEngine
{
    HierarchyPanel::HierarchyPanel()
            : m_HadRecentDroppedEntity(entt::null)
            , m_DoubleClicked(entt::null)
    {
        m_Name       = "Hierarchy###hierarchy";
        m_SimpleName = "Hierarchy";
    }

    void HierarchyPanel::DrawNode(entt::entity node, entt::registry& registry)
    {

        bool show = true;

        if(!registry.valid(node))
            return;

        Entity nodeEntity = { node, gEngine->GetLevelManager()->GetCurrentLevel() };

        static const char* defaultName     = "Entity";
        const NameComponent* nameComponent = registry.try_get<NameComponent>(node);
        const char* name                   = nameComponent ? nameComponent->name.c_str() : defaultName; // StringUtility::ToString(entt::to_integral(node));

        if(m_HierarchyFilter.IsActive())
        {
            if(!m_HierarchyFilter.PassFilter(name))
            {
                show = false;
            }
        }

        if(show)
        {
            ImGui::PushID((int)node);
            auto hierarchyComponent = registry.try_get<Hierarchy>(node);
            bool noChildren         = true;

            if(hierarchyComponent != nullptr && hierarchyComponent->First() != entt::null)
                noChildren = false;

            ImGuiTreeNodeFlags nodeFlags = ((m_Editor->IsSelected(node)) ? ImGuiTreeNodeFlags_Selected : 0);

            nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

            if(noChildren)
            {
                nodeFlags |= ImGuiTreeNodeFlags_Leaf;
            }

            // auto activeComponent = registry.try_get<ActiveComponent>(node);
            bool active = Entity(node, m_Editor->GetCurrentLevel()).Active(); // activeComponent ? activeComponent->active : true;

            if(!active)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

            bool doubleClicked = false;
            if(node == m_DoubleClicked)
            {
                doubleClicked = true;
            }

            if(doubleClicked)
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1.0f, 2.0f });

            if(m_HadRecentDroppedEntity == node)
            {
                ImGui::SetNextItemOpen(true);
                m_HadRecentDroppedEntity = entt::null;
            }

            std::string icon = ICON_MDI_CUBE_OUTLINE;
            auto& iconMap    = m_Editor->GetComponentIconMap();

            if(registry.all_of<Camera>(node))
            {
                if(iconMap.find(typeid(Camera).hash_code()) != iconMap.end())
                    icon = iconMap[typeid(Camera).hash_code()];
            }
            if(registry.all_of<LuaScriptComponent>(node))
            {
                if(iconMap.find(typeid(LuaScriptComponent).hash_code()) != iconMap.end())
                    icon = iconMap[typeid(LuaScriptComponent).hash_code()];
            }
            else if(registry.all_of<Light>(node))
            {
                if(iconMap.find(typeid(Light).hash_code()) != iconMap.end())
                    icon = iconMap[typeid(Light).hash_code()];
            }
            else if(registry.all_of<Environment>(node))
            {
                if(iconMap.find(typeid(Environment).hash_code()) != iconMap.end())
                    icon = iconMap[typeid(Environment).hash_code()];
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetIconColour());
            // ImGui::BeginGroup();
            bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entt::to_integral(node), nodeFlags, "%s", icon.c_str());
            {
                // Allow clicking of icon and text. Need twice as they are separated
                if(ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
                {
                    bool ctrlDown = gEngine->GetInput()->GetKeyHeld(KeyCode::LeftControl) || gEngine->GetInput()->GetKeyHeld(KeyCode::RightControl) || gEngine->GetInput()->GetKeyHeld(KeyCode::LeftSuper);
                    if(!ctrlDown)
                        m_Editor->ClearSelected();

                    if(!m_Editor->IsSelected(node))
                        m_Editor->SetSelected(node);
                    else
                        m_Editor->UnSelect(node);
                }
                else if(m_DoubleClicked == node && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                    m_DoubleClicked = entt::null;
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if(!doubleClicked)
            {
                bool isPrefab = false;
                if(registry.any_of<PrefabComponent>(node))
                    isPrefab = true;
                else
                {
                    auto Parent = nodeEntity.GetParent();
                    while(Parent && Parent.Valid())
                    {
                        if(Parent.HasComponent<PrefabComponent>())
                        {
                            isPrefab = true;
                            Parent   = {};
                        }
                        else
                        {
                            Parent = Parent.GetParent();
                        }
                    }
                }

                if(isPrefab)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
                ImGui::TextUnformatted(name);
                if(isPrefab)
                    ImGui::PopStyleColor();
            }
            // ImGui::EndGroup();

            if(doubleClicked)
            {
                static char objName[INPUT_BUF_SIZE];
                strcpy(objName, name);
                ImGui::PushItemWidth(-1);
                if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
                    registry.get_or_emplace<NameComponent>(node).name = objName;
                ImGui::PopStyleVar();
            }

            if(!active)
                ImGui::PopStyleColor();

            bool deleteEntity = false;
            if(ImGui::BeginPopupContextItem(name))
            {
                if(ImGui::Selectable("Copy"))
                {
                    if(!m_Editor->IsSelected(node))
                    {
                        m_Editor->SetCopiedEntity(node);
                    }
                    for(auto entity : m_Editor->GetSelected())
                        m_Editor->SetCopiedEntity(entity);
                }

                if(ImGui::Selectable("Cut"))
                {
                    for(auto entity : m_Editor->GetSelected())
                        m_Editor->SetCopiedEntity(node, true);
                }

                if(m_Editor->GetCopiedEntity().size() > 0 && registry.valid(m_Editor->GetCopiedEntity().front()))
                {
                    if(ImGui::Selectable("Paste"))
                    {
                        for(auto entity : m_Editor->GetCopiedEntity())
                        {
                            auto scene          = gEngine->GetLevelManager()->GetCurrentLevel();
                            Entity copiedEntity = { entity, scene };
                            if(!copiedEntity.Valid())
                            {
                                m_Editor->SetCopiedEntity(entt::null);
                            }
                            else
                            {
                                scene->DuplicateEntity(copiedEntity, { node, scene });

                                if(m_Editor->GetCutCopyEntity())
                                    deleteEntity = true;
                            }
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("Paste");
                }

                ImGui::Separator();

                if(ImGui::Selectable("Duplicate"))
                {
                    auto scene = gEngine->GetLevelManager()->GetCurrentLevel();
                    scene->DuplicateEntity({ node, scene });
                }
                if(ImGui::Selectable("Delete"))
                    deleteEntity = true;
                // if(m_Editor->IsSelected(node))
                //   m_Editor->UnSelect(node);
                ImGui::Separator();
                if(ImGui::Selectable("Rename"))
                    m_DoubleClicked = node;
                ImGui::Separator();

                if(ImGui::Selectable("Add Child"))
                {
                    auto scene = gEngine->GetLevelManager()->GetCurrentLevel();
                    auto child = scene->CreateEntity();
                    child.SetParent({ node, scene });
                }

                if(ImGui::Selectable("Zoom to"))
                {
                    // if(gEngine->GetEditorState() == EditorState::Preview)
                    {
                        auto transform = registry.try_get<Transform>(node);
                        if(transform)
                            m_Editor->FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
                    }
                }
                ImGui::EndPopup();
            }

            if(!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                auto ptr = node;
                ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(entt::entity*));
                ImGui::Text(ICON_MDI_ARROW_UP);
                ImGui::EndDragDropSource();
            }

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if(payload != NULL && payload->IsDataType("Drag_Entity"))
            {
                bool acceptable;

                ASSERT(payload->DataSize != sizeof(entt::entity*), "Error ImGUI drag entity");
                auto entity             = *reinterpret_cast<entt::entity*>(payload->Data);
                auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
                if(hierarchyComponent != nullptr)
                {
                    acceptable = entity != node && (!IsParentOfEntity(entity, node, registry)) && (hierarchyComponent->Parent() != node);
                }
                else
                    acceptable = entity != node;

                if(ImGui::BeginDragDropTarget())
                {
                    // Drop directly on to node and append to the end of it's children list.
                    if(ImGui::AcceptDragDropPayload("Drag_Entity"))
                    {
                        if(acceptable)
                        {
                            if(hierarchyComponent)
                                Hierarchy::Reparent(entity, node, registry, *hierarchyComponent);
                            else
                            {
                                registry.emplace<Hierarchy>(entity, node);
                            }
                            m_HadRecentDroppedEntity = node;
                        }
                    }

                    ImGui::EndDragDropTarget();
                }

                if(m_Editor->IsSelected(entity))
                    m_Editor->UnSelect(entity);
            }

            if(ImGui::IsItemClicked() && !deleteEntity)
                m_Editor->SetSelected(node);
            else if(m_DoubleClicked == node && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                m_DoubleClicked = entt::null;

            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
            {
                m_DoubleClicked = node;
                if(gEngine->GetEditorState() == EditorState::Preview)
                {
                    auto transform = registry.try_get<Transform>(node);
                    if(transform)
                        m_Editor->FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
                }
            }

            if(deleteEntity)
            {
                DestroyEntity(node, registry);
                if(nodeOpen)
                    ImGui::TreePop();

                ImGui::PopID();
                return;
            }

            if(m_SelectUp)
            {
                if(!m_Editor->GetSelected().empty() && m_Editor->GetSelected().front() == node && registry.valid(m_CurrentPrevious))
                {
                    m_SelectUp = false;
                    m_Editor->SetSelected(m_CurrentPrevious);
                }
            }

            if(m_SelectDown)
            {
                if(!m_Editor->GetSelected().empty() && registry.valid(m_CurrentPrevious) && m_CurrentPrevious == m_Editor->GetSelected().front())
                {
                    m_SelectDown = false;
                    m_Editor->SetSelected(node);
                }
            }

            m_CurrentPrevious = node;

#if 1
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(ICON_MDI_EYE).x * 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
            if(ImGui::Button(active ? ICON_MDI_EYE : ICON_MDI_EYE_OFF))
            {
                auto& activeComponent = registry.get_or_emplace<ActiveComponent>(node);

                activeComponent.active = !active;
            }
            ImGui::PopStyleColor();
#endif

            if(nodeOpen == false)
            {
                ImGui::PopID();
                return;
            }

            const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
            const float SmallOffsetX    = 6.0f * gEngine->GetWindowDPI();
            ImDrawList* drawList        = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
            ImVec2 verticalLineEnd = verticalLineStart;

            if(!noChildren)
            {
                entt::entity child = hierarchyComponent->First();
                while(child != entt::null && registry.valid(child))
                {
                    float HorizontalTreeLineSize = 20.0f * gEngine->GetWindowDPI(); // chosen arbitrarily
                    auto currentPos              = ImGui::GetCursorScreenPos();
                    ImGui::Indent(10.0f);

                    auto childHerarchyComponent = registry.try_get<Hierarchy>(child);

                    if(childHerarchyComponent)
                    {
                        entt::entity firstChild = childHerarchyComponent->First();
                        if(firstChild != entt::null && registry.valid(firstChild))
                        {
                            HorizontalTreeLineSize *= 0.1f;
                        }
                    }
                    DrawNode(child, registry);
                    ImGui::Unindent(10.0f);

                    const ImRect childRect = ImRect(currentPos, currentPos + ImVec2(0.0f, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y));

                    const float midpoint = (childRect.Min.y + childRect.Max.y) * 0.5f;
                    drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                    verticalLineEnd.y = midpoint;

                    if(registry.valid(child))
                    {
                        auto hierarchyComponent = registry.try_get<Hierarchy>(child);
                        child                   = hierarchyComponent ? hierarchyComponent->Next() : entt::null;
                    }
                }
            }

            drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

            ImGui::TreePop();
            ImGui::PopID();
        }
    }

    void HierarchyPanel::DestroyEntity(entt::entity entity, entt::registry& registry)
    {

        auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
        if(hierarchyComponent)
        {
            entt::entity child = hierarchyComponent->First();
            while(child != entt::null)
            {
                auto hierarchyComponent = registry.try_get<Hierarchy>(child);
                auto next               = hierarchyComponent ? hierarchyComponent->Next() : entt::null;
                DestroyEntity(child, registry);
                child = next;
            }
        }
        registry.destroy(entity);
    }

    bool HierarchyPanel::IsParentOfEntity(entt::entity entity, entt::entity child, entt::registry& registry)
    {

        auto nodeHierarchyComponent = registry.try_get<Hierarchy>(child);
        if(nodeHierarchyComponent)
        {
            auto parent = nodeHierarchyComponent->Parent();
            while(parent != entt::null)
            {
                if(parent == entity)
                {
                    return true;
                }
                else
                {
                    nodeHierarchyComponent = registry.try_get<Hierarchy>(parent);
                    parent                 = nodeHierarchyComponent ? nodeHierarchyComponent->Parent() : entt::null;
                }
            }
        }

        return false;
    }

    void HierarchyPanel::OnImGui()
    {

        auto flags        = ImGuiWindowFlags_NoCollapse;
        m_CurrentPrevious = entt::null;
        m_SelectUp        = false;
        m_SelectDown      = false;

        m_SelectUp   = gEngine->GetInput()->GetKeyPressed(KeyCode::Up);
        m_SelectDown = gEngine->GetInput()->GetKeyPressed(KeyCode::Down);

        ImGui::Begin(m_Name.c_str(), &m_Active, flags);
        {
            auto scene = gEngine->GetLevelManager()->GetCurrentLevel();

            if(!scene)
            {
                ImGui::End();
                return;
            }
            auto& registry = scene->GetRegistry();

            auto AddEntity = [scene]()
            {
                if(ImGui::Selectable("Add Empty Entity"))
                {
                    scene->CreateEntity();
                }

                if(ImGui::Selectable("Add Light"))
                {
                    auto entity = scene->CreateEntity("Light");
                    entity.AddComponent<Light>();
                    entity.GetOrAddComponent<Transform>();
                }

                if(ImGui::Selectable("Add Rigid Body"))
                {
                    auto entity = scene->CreateEntity("RigidBody");
                    entity.AddComponent<RigidBody3DComponent>();
                    entity.GetOrAddComponent<Transform>();
                    entity.AddComponent<AxisConstraintComponent>(entity, Axes::XZ);
                    entity.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetCollisionShape(CollisionShapeType::CollisionCuboid);
                }

                if(ImGui::Selectable("Add Camera"))
                {
                    auto entity = scene->CreateEntity("Camera");
                    entity.AddComponent<Camera>();
                    entity.GetOrAddComponent<Transform>();
                }


                if(ImGui::Selectable("Add Lua Script"))
                {
                    auto entity = scene->CreateEntity("LuaScript");
                    entity.AddComponent<LuaScriptComponent>();
                }
            };

            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));

            if(ImGui::Button(ICON_MDI_PLUS))
            {
                // Add Entity Menu
                ImGui::OpenPopup("AddEntity");
            }

            if(ImGui::BeginPopup("AddEntity"))
            {
                AddEntity();
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
            ImGui::SameLine();

            {
                ImGuiUtility::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGuiUtility::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGuiUtility::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
                m_HierarchyFilter.Draw("##HierarchyFilter", ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);
                ImGuiUtility::DrawItemActivityOutline(2.0f, false);
            }

            if(!m_HierarchyFilter.IsActive())
            {
                ImGui::SameLine();
                ImGuiUtility::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                ImGui::SetCursorPosX(ImGui::GetFontSize() * 4.0f);
                ImGuiUtility::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));
                ImGui::TextUnformatted("Search...");
            }

            ImGui::PopStyleColor();
            ImGui::Unindent();

            ImGui::Separator();

            ImGui::BeginChild("Nodes");

            if(ImGui::BeginPopupContextWindow())
            {
                if(!m_Editor->GetCopiedEntity().empty() && registry.valid(m_Editor->GetCopiedEntity().front()))
                {
                    if(ImGui::Selectable("Paste"))
                    {
                        for(auto entity : m_Editor->GetCopiedEntity())
                        {
                            auto scene          = gEngine->GetLevelManager()->GetCurrentLevel();
                            Entity copiedEntity = { entity, scene };
                            if(!copiedEntity.Valid())
                            {
                                m_Editor->SetCopiedEntity(entt::null);
                            }
                            else
                            {
                                scene->DuplicateEntity(copiedEntity);

                                if(m_Editor->GetCutCopyEntity())
                                {
                                    DestroyEntity(entity, registry);
                                }
                            }
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("Paste");
                }

                ImGui::Separator();

                AddEntity();

                ImGui::EndPopup();
            }
            {
                if(ImGui::BeginDragDropTarget())
                {
                    if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
                    {
                        ASSERT(payload->DataSize != sizeof(entt::entity*), "Error ImGUI drag entity");
                        auto entity             = *reinterpret_cast<entt::entity*>(payload->Data);
                        auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
                        if(hierarchyComponent)
                        {
                            Hierarchy::Reparent(entity, entt::null, registry, *hierarchyComponent);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                //                auto draw_list = ImGui::GetWindowDrawList();
                //                auto availSize = ImGui::GetContentRegionAvail();
                //                ImVec2 min = ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetStyle().ItemSpacing.x, ImGui::GetScrollY());
                //                draw_list->AddRectFilled(min, min + availSize, ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_FrameBg)) - ImVec4(0.15f, 0.15f, 0.15f, 0.0f)));

                ImGui::Indent();

                // ImGuiUtility::AlternatingRowsBackground(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y);

                for(auto [entity] : registry.storage<entt::entity>().each())
                {
                    if(registry.valid(entity))
                    {
                        auto hierarchyComponent = registry.try_get<Hierarchy>(entity);

                        if(!hierarchyComponent || hierarchyComponent->Parent() == entt::null)
                            DrawNode(entity, registry);
                    }
                }

                // Only supports one scene
                ImVec2 min_space = ImGui::GetWindowContentRegionMin();
                ImVec2 max_space = ImGui::GetWindowContentRegionMax();

                float yOffset = Maths::Max(45.0f, ImGui::GetScrollY()); // Dont include search bar
                min_space.x += ImGui::GetWindowPos().x + 1.0f;
                min_space.y += ImGui::GetWindowPos().y + 1.0f + yOffset;
                max_space.x += ImGui::GetWindowPos().x - 1.0f;
                max_space.y += ImGui::GetWindowPos().y - 1.0f + ImGui::GetScrollY();
                ImRect bb { min_space, max_space };

                const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                if(payload != NULL && payload->IsDataType("Drag_Entity"))
                {
                    bool acceptable = false;

                    ASSERT(payload->DataSize != sizeof(entt::entity*), "Error ImGUI drag entity");
                    auto entity             = *reinterpret_cast<entt::entity*>(payload->Data);
                    auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
                    if(hierarchyComponent)
                    {
                        acceptable = hierarchyComponent->Parent() != entt::null;
                    }

                    if(acceptable && ImGui::BeginDragDropTargetCustom(bb, ImGui::GetID("Panel Hierarchy")))
                    {
                        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
                        {
                            ASSERT(payload->DataSize != sizeof(entt::entity*), "Error ImGUI drag entity");
                            auto entity             = *reinterpret_cast<entt::entity*>(payload->Data);
                            auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
                            if(hierarchyComponent)
                            {
                                Hierarchy::Reparent(entity, entt::null, registry, *hierarchyComponent);
                                Entity e(entity, scene);
                                e.RemoveComponent<Hierarchy>();
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
} // NekoEngine