#include "VulkanDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    VulkanCommandPool::VulkanCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        createInfo.flags = flags;

        vkCreateCommandPool(gVulkanContext.GetDevice()->GetHandle(), &createInfo, nullptr, &handle);
    }

    void VulkanCommandPool::Destroy()
    {
        vkDestroyCommandPool(gVulkanContext.GetDevice()->GetHandle(), handle, nullptr);
    }


} // NekoEngine