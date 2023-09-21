#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    VulkanFence::VulkanFence(bool signaled)
    {
        isSignaled = signaled;

        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        vkCreateFence(GET_DEVICE(), &info, nullptr, &handle);
    }

    VulkanFence::~VulkanFence()
    {
        vkDestroyFence(GET_DEVICE(), handle, nullptr);
    }

    bool VulkanFence::IsSignaled()
    {
        if(isSignaled)
        {
            return true;
        }
        else
        {
            return CheckState();
        }
    }

    bool VulkanFence::CheckState()
    {
        VkResult result = vkGetFenceStatus(GET_DEVICE(), handle);
        if(result == VK_SUCCESS)
        {
            isSignaled = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool VulkanFence::Wait()
    {
        if(!isSignaled) LOG("Waiting for unsignaled fence!");
        auto result = vkWaitForFences(GET_DEVICE(), 1, &handle, VK_TRUE, UINT64_MAX);
        if(result == VK_SUCCESS)
        {
            isSignaled = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    void VulkanFence::Reset()
    {
        if(isSignaled)
        {
            VK_CHECK_RESULT(vkResetFences(GET_DEVICE(), 1, &handle), "Failed to reset fence!");
        }
        isSignaled = false;
    }

    void VulkanFence::WaitAndReset()
    {
        if(!isSignaled) Wait();

        Reset();
    }

} // NekoEngine