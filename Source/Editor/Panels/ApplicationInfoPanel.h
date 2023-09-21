#pragma once
#include "EditorPanel.h"
namespace NekoEngine
{

    class ApplicationInfoPanel : public EditorPanel
    {
    public:
        ApplicationInfoPanel();
        ~ApplicationInfoPanel() = default;

        void OnImGui() override;

        std::vector<float> m_FPSData;
    };

} // NekoEngine

