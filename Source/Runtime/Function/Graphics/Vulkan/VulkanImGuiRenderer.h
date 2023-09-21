#pragma once

#define VK_NO_PROTOTYPES
#include "RHI/ImGuiRenderer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFrameBuffer.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"
#include "ImGui/imgui_impl_vulkan.h"

namespace NekoEngine
{

    class VulkanImGuiRenderer : public ImGuiRenderer
    {
    private:
        void* m_WindowHandle = nullptr;
        uint32_t m_Width;
        uint32_t m_Height;
        VulkanFramebuffer* m_Framebuffers[3] = {};
        VulkanRenderPass* m_Renderpass = nullptr;
        VulkanTexture2D* m_FontTexture = nullptr;
        bool m_ClearScreen;
    public:
        VulkanImGuiRenderer(uint32_t width, uint32_t height, bool clearScreen);
        ~VulkanImGuiRenderer();

        void Init() override;
        void Render(CommandBuffer* commandBuffer) override;
        void OnResize(uint32_t width, uint32_t height) override;
        void Clear() override;

        void FrameRender(ImGui_ImplVulkanH_Window* wd);
        void SetupVulkanWindowData(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
        bool Implemented() const override { return true; }
        void RebuildFontTexture() override;
    };

} // NekoEngine

