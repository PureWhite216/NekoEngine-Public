#pragma once

#include "EditorPanel.h"
#include "RHI/Texture.h"
#include "Renderer/SceneRenderer.h"

namespace NekoEngine
{

    class GameViewPanel : public EditorPanel
    {
    public:
        GameViewPanel();

        ~GameViewPanel() = default;

        void OnImGui() override;

        void OnNewLevel(Level* level) override;

        void OnRender() override;

        void DrawGizmos(float width, float height, float xpos, float ypos, Level* level);

        void Resize(uint32_t width, uint32_t height);

        void ToolBar();

    private:
        SharedPtr<Texture2D> m_GameViewTexture = nullptr;
        Level* m_CurrentLevel = nullptr;
        uint32_t m_Width, m_Height;
        UniquePtr<SceneRenderer> m_SceneRenderer;
        bool m_GameViewVisible = false;
        bool m_ShowStats = false;
    };

} // NekoEngine
