#include "EditorPanel.h"
#include "ImGui/ImGuiEnttEditor.hpp"

namespace NekoEngine
{

    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel();
        ~InspectorPanel() = default;

        void OnNewLevel(Level* level) override;
        void OnImGui() override;
        void SetDebugMode(bool mode);
        bool GetIsDebugMode() const { return m_DebugMode; };

    private:
        MM::ImGuiEntityEditor<entt::entity> m_EnttEditor;
        bool m_DebugMode = false;
    };

} // NekoEngine
