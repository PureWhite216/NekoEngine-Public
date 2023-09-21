#pragma once
#include "Core.h"
#include "Renderable/Mesh.h"
#include "IRenderer.h"

namespace NekoEngine
{
    struct UBOFrag
    {
        glm::vec4 cameraPos;
        glm::vec4 cameraForward;
        float Near;
        float Far;
        float maxDistance;
        float p1;
    };

    class Level;

    class GridRenderer : public IRenderer
    {
    private:
        uint32_t m_CurrentBufferID = 0;
        Mesh* m_Quad;

        float m_GridRes     = 1.0f;
        float m_GridSize    = 1.0f;
        float m_MaxDistance = 10000.0f;

    public:
        GridRenderer(uint32_t width, uint32_t height);
        ~GridRenderer();

        void Init() override;
        void BeginScene(Level* level, Camera* overrideCamera, Transform* overrideCameraTransform) override;
        void OnResize(uint32_t width, uint32_t height) override;
        void CreateGraphicsPipeline();
        void UpdateUniformBuffer();

        void Begin() override;
        void Submit(const RenderCommand& command) override {};
        void SubmitMesh(Mesh* mesh, Material* material, const glm::mat4& transform, const glm::mat4& textureMatrix) override {};
        void EndScene() override {};
        void End() override;
        void Present() override {};
        void RenderScene() override;
        void PresentToScreen() override { }

        void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;
        void OnImGui() override;
    };

} // NekoEngine

