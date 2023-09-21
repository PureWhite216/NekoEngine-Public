#pragma once
#include "EditorPanel.h"

namespace NekoEngine
{

    class ProjectSettingsPanel : public EditorPanel
    {
    public:
        ProjectSettingsPanel();
        ~ProjectSettingsPanel() = default;

        void OnNewLevel(Level* level) override { m_CurrentLevel = level; }
        void OnImGui() override;

    private:
        Level* m_CurrentLevel = nullptr;
        bool m_NameUpdated    = false;
        std::string m_ProjectName;
    };

} // NekoEngine

