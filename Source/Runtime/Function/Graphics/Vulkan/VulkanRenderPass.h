#pragma once
#include "Vk.h"
#include "RHI/RenderPass.h"
namespace NekoEngine
{
    VkAttachmentDescription GetAttachmentDescription(TextureType type, Texture* texture, bool isClear = true);
    VkSubpassContents SubPassContents2Vk(SubPassContents contents);

    class VulkanRenderPass : public RenderPass
    {
    private:
        VkRenderPass handle = VK_NULL_HANDLE;
        VkClearValue* clearValues = nullptr;
        uint32_t clearValueCount = 0;
        uint32_t colorAttachmentCount = 0;
        bool isDepthOnly = false;
        bool isClearDepth = false;
        bool isSwapChainTarget = false;

    public:
        VulkanRenderPass(const RenderPassDesc& renderPassDesc);
        ~VulkanRenderPass() override;

        bool Init(const RenderPassDesc& renderPassDesc);
        void BeginRenderpass(CommandBuffer* commandBuffer, Color clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const override;
        void EndRenderpass(CommandBuffer* commandBuffer) override;

        const VkRenderPass& GetHandle() const { return handle; };
        int GetAttachmentCount() const override { return clearValueCount; };
        int GetColourAttachmentCount() const { return colorAttachmentCount; }

    };

} // NekoEngine