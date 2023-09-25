#pragma once

#include "Core.h"
#include "RHI/RenderPass.h"
#include "Renderable/Mesh.h"
#include "Math/Frustum.h"
#include "Transform.h"

namespace NekoEngine
{
    struct RenderCommand
    {
        Mesh* mesh = nullptr;
        Material* material = nullptr;
        Pipeline* pipeline = nullptr;
        glm::mat4 transform;
        glm::mat4 textureMatrix;
        bool animated = false;
    };

    typedef std::vector<RenderCommand> CommandQueue;

    class RenderList;

    class Level;

    class Camera;

    class IRenderer
    {
    protected:
        Camera* m_Camera = nullptr;
        Transform* m_CameraTransform = nullptr;

        SharedPtr<Pipeline> m_Pipeline;
        std::vector<SharedPtr<DescriptorSet>> m_DescriptorSet;

        std::vector<DescriptorSet*> m_CurrentDescriptorSets;
        SharedPtr<Shader> m_Shader = nullptr;

        uint32_t m_ScreenBufferWidth = 0, m_ScreenBufferHeight = 0;
        CommandQueue m_CommandQueue;
        Texture* m_RenderTexture = nullptr;
        Texture* m_DepthTexture = nullptr;

        Frustum m_Frustum;
        glm::vec4 m_ClearColour;

        int m_RenderPriority = 0;
        bool m_ScreenRenderer = true;
        bool m_ShouldRender = true;

    public:
        virtual ~IRenderer() = default;

        virtual void RenderScene() = 0;

        virtual void Init() = 0;

        virtual void Begin() = 0;

        virtual void BeginScene(Level* level, Camera* overrideCamera, Transform* overrideCameraTransform) = 0;

        virtual void Submit(const RenderCommand &command)
        {};

        virtual void
        SubmitMesh(Mesh* mesh, Material* material, const glm::mat4 &transform, const glm::mat4 &textureMatrix)
        {};

        virtual void EndScene() = 0;

        virtual void End() = 0;

        virtual void Present() = 0;

        virtual void PresentToScreen() = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;

        virtual void OnImGui()
        {};

        virtual void SetScreenBufferSize(uint32_t width, uint32_t height)
        {
            // ASSERT(width == 0 || height == 0, "Width or Height 0!");

            m_ScreenBufferWidth = width;
            m_ScreenBufferHeight = height;
        }

        virtual void SetRenderTarget(Texture* texture, bool rebuildFramebuffer = true)
        {
            m_RenderTexture = texture;
        }

        virtual void SetDepthTarget(Texture* texture)
        {
            m_DepthTexture = texture;
        }

        Texture* GetRenderTarget() const
        {
            return m_RenderTexture;
        }

        const SharedPtr<Shader> &GetShader() const
        {
            return m_Shader;
        }

        void SetCamera(Camera* camera, Transform* transform)
        {
            m_Camera = camera;
            m_CameraTransform = transform;
        }

        int GetRenderPriority() const
        {
            return m_RenderPriority;
        }

        void SetRenderPriority(int priority)
        {
            m_RenderPriority = priority;
        }

        bool GetScreenRenderer()
        {
            return m_ScreenRenderer;
        }

        bool GetShouldRender()
        {
            return m_ShouldRender;
        }
    };

} // NekoEngine

