#pragma once
#include "EditorPanel.h"

namespace NekoEngine
{

    class SceneSettingsPanel : public EditorPanel
    {
    public:
        SceneSettingsPanel();
        ~SceneSettingsPanel() = default;

        void OnNewLevel(Level* level) override { m_CurrentLevel = level; }
        void OnImGui() override;

    private:
        Level* m_CurrentLevel = nullptr;
        bool m_NameUpdated    = false;
        std::string m_LevelName;
    };

} // NekoEngine

