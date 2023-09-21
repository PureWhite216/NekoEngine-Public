#pragma once
#include "Vk.h"
namespace NekoEngine
{

    class VulkanCommandPool
    {
    private:
        VkCommandPool handle;
    public:
        VulkanCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
        ~VulkanCommandPool() = default;

        void Destroy();

        const VkCommandPool& GetHandle() const
        {
            return handle;
        }
    };

}