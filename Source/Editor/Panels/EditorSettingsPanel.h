#pragma once
#include "EditorPanel.h"

namespace NekoEngine
{

    class EditorSettingsPanel : public EditorPanel
    {
    public:
        EditorSettingsPanel();
        ~EditorSettingsPanel() = default;

        void OnNewLevel(Level* level) override { m_CurrentLevel = level; }
        void OnImGui() override;

    private:
        Level* m_CurrentLevel = nullptr;
    };

} // NekoEngine
