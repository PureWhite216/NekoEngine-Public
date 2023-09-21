#pragma once
#include "Vk.h"
#include "RHI/Framebuffer.h"
#include "VulkanDevice.h"
namespace NekoEngine
{

    class VulkanFramebuffer : public Framebuffer
    {
    private:
        VkFramebuffer handle = VK_NULL_HANDLE;

    public:
        VulkanFramebuffer(const FramebufferDesc& desc);
        ~VulkanFramebuffer() override;


        const VkFramebuffer& GetHandle() const { return handle; };
    };

} // NekoEngine
