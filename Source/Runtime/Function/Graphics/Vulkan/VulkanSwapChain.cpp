#include "VulkanSwapChain.h"
#include "VulkanTexture.h"
#include "Window/GLFWWindow.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "Engine.h"
#include "VulkanRenderer.h"
namespace NekoEngine
{
    VulkanSwapChain::VulkanSwapChain(uint32_t _width, uint32_t _height) : width(_width), height(_height)
    {
    }

    bool VulkanSwapChain::Init(bool vsync, Window* windowHandle)
    {
        isVSync = vsync;

        if(surface == VK_NULL_HANDLE)
        {
            surface = CreateSurface(gVulkanContext.GetVkInstance(), windowHandle);
        }

        bool success = Init(isVSync);

        return success;
    }

    bool VulkanSwapChain::Init(bool vsync)
    {
//        this->isVSync = vsync;
//
//        if(surface == VK_NULL_HANDLE)
//        {
//            LOG("VulkanSwapChain::Init() failed: surface is null");
//        }

        InitImageFormatAndColourSpace();

        VkBool32 queueIndexSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle(), gVulkanContext.GetDevice()->GetPhysicalDevice()->GetGraphicsFamilyIndex(), surface, &queueIndexSupported);
        if(queueIndexSupported == VK_FALSE)
        {
            RUNTIME_ERROR("no graphics queue family index support");
        }

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle(), surface, &surfaceCapabilities) != VK_SUCCESS)
        {
            RUNTIME_ERROR("failed to get surface capabilities!");
        }

        uint32_t numPresentModes;
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle(), surface, &numPresentModes, nullptr) != VK_SUCCESS)
        {
            RUNTIME_ERROR("failed to get surface present modes!");
        }

        ArrayList<VkPresentModeKHR> presentModes(numPresentModes);
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle(), surface, &numPresentModes, presentModes.data()) != VK_SUCCESS)
        {
            RUNTIME_ERROR("failed to get surface present modes!");
        }

        VkExtent2D swapChainExtent;

        swapChainExtent.width = width;
        swapChainExtent.height = height;

        VkPresentModeKHR swapChainPresentMode;
        if(this->isVSync)
        {
            swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
        else
        {
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        swapChainBufferNum = Maths::Max(MAX_BUFFER_COUNT, (int)surfaceCapabilities.maxImageCount);

        VkSurfaceTransformFlagBitsKHR preTransform;
        if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = surfaceCapabilities.currentTransform;
        }

        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        //TODO: check if VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR is supported

        VkSwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = surface;
        swapChainCreateInfo.minImageCount = swapChainBufferNum;
        swapChainCreateInfo.imageFormat = colorFormat;
        swapChainCreateInfo.imageColorSpace = colorSpace;
        swapChainCreateInfo.imageExtent = swapChainExtent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
        swapChainCreateInfo.preTransform = preTransform;
        swapChainCreateInfo.compositeAlpha = compositeAlpha;
        swapChainCreateInfo.presentMode = swapChainPresentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = oldHandle;

        VK_CHECK_RESULT(vkCreateSwapchainKHR(gVulkanContext.GetDevice()->GetHandle(), &swapChainCreateInfo, nullptr, &handle), "failed to create swap chain!");

        // Clear old swapChain
        if(oldHandle != VK_NULL_HANDLE)
        {
            for(int i = 0 ; i < swapChainBufferNum; i++)
            {
                if(bufferData[i].MainCommandBuffer->GetState() == CommandBufferState::Submitted)
                {
                    bufferData[i].MainCommandBuffer->Wait();
                }
                bufferData[i].MainCommandBuffer->Reset();

                delete swapChainBuffers[i];
                vkDestroySemaphore(gVulkanContext.GetDevice()->GetHandle(), bufferData[i].PresentSemaphore, nullptr);
                bufferData[i].PresentSemaphore = VK_NULL_HANDLE;
            }
            vkDestroySwapchainKHR(gVulkanContext.GetDevice()->GetHandle(), oldHandle, nullptr);
            oldHandle = VK_NULL_HANDLE;
        }

        uint32_t imageCount;
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(gVulkanContext.GetDevice()->GetHandle(), handle, &imageCount, nullptr), "failed to get swap chain images!");

        ArrayList<VkImage> tmpSwapChainImages(imageCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(gVulkanContext.GetDevice()->GetHandle(), handle, &imageCount, tmpSwapChainImages.data()), "failed to get swap chain images!");

        for(uint32_t i = 0; i < imageCount; i++)
        {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = tmpSwapChainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = colorFormat;
            imageViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            imageViewCreateInfo.flags = 0;
            imageViewCreateInfo.image  = tmpSwapChainImages[i];

            VkImageView imageView;
            VK_CHECK_RESULT(vkCreateImageView(gVulkanContext.GetDevice()->GetHandle(), &imageViewCreateInfo, nullptr, &imageView), "failed to create image view!");
            auto* swapChainBuffer = new VulkanTexture2D(tmpSwapChainImages[i], imageView, colorFormat, width, height);
            swapChainBuffer->TransitionImage(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, nullptr);
            swapChainBuffers.push_back(swapChainBuffer);
        }

        CreateFrameData();

        return true;
    }

    void VulkanSwapChain::CreateFrameData()
    {
        for(uint32_t i = 0; i < swapChainBufferNum; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreCreateInfo.pNext = nullptr;
            semaphoreCreateInfo.flags = 0;

            if(bufferData[i].PresentSemaphore == VK_NULL_HANDLE)
            {
                VK_CHECK_RESULT(vkCreateSemaphore(gVulkanContext.GetDevice()->GetHandle(), &semaphoreCreateInfo, nullptr, &bufferData[i].PresentSemaphore), "failed to create semaphore!");
            }
            if(!bufferData[i].MainCommandBuffer)
            {
                bufferData[i].CommandPool = MakeShared<VulkanCommandPool>(gVulkanContext.GetDevice()->GetPhysicalDevice()->GetGraphicsFamilyIndex(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                bufferData[i].MainCommandBuffer = MakeShared<VulkanCommandBuffer>();

            }
        }
    }

    VulkanSwapChain::~VulkanSwapChain()
    {
        vkDeviceWaitIdle(gVulkanContext.GetDevice()->GetHandle());

        for(int i = 0 ; i < swapChainBuffers.size() ; i++)
        {
            vkDestroySemaphore(gVulkanContext.GetDevice()->GetHandle(), bufferData[i].PresentSemaphore, nullptr);

            bufferData[i].MainCommandBuffer->Flush();
            bufferData[i].MainCommandBuffer = nullptr;
            bufferData[i].CommandPool = nullptr;

            delete swapChainBuffers[i];
        }

        vkDestroySwapchainKHR(gVulkanContext.GetDevice()->GetHandle(), handle, nullptr);

        if(surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(gVulkanContext.GetVkInstance(), surface, nullptr);
    }

    VkSurfaceKHR VulkanSwapChain::CreateSurface(VkInstance vkInstance, Window* window)
    {
        VkSurfaceKHR surfaceKHR;
        if(window->GetHandle() == nullptr)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        if(glfwCreateWindowSurface(gVulkanContext.GetVkInstance(), (GLFWwindow*)(window->GetHandle()), nullptr, &surfaceKHR) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }

        return surfaceKHR;
    }

    void VulkanSwapChain::InitImageFormatAndColourSpace()
    {
        VkPhysicalDevice physicalDevice = gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle();

        uint32_t formatCount = 0;
        auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to get surface formats!");
        }

        LOG("VulkanSwapChain::InitImageFormatAndColourSpace()");

        ArrayList<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to get surface formats!");
        }

        if(formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
            colorSpace = surfaceFormats[0].colorSpace;
        }
        else
        {
            bool found = false;
            for(const auto& surfaceFormat : surfaceFormats)
            {
                if(surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    colorFormat = surfaceFormat.format;
                    colorSpace = surfaceFormat.colorSpace;
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                colorFormat = surfaceFormats[0].format;
                colorSpace = surfaceFormats[0].colorSpace;
            }
        }
    }

    Texture* VulkanSwapChain::GetCurrentImage()
    {
        return (Texture*)swapChainBuffers[acquireImageIndex];
    }

    Texture* VulkanSwapChain::GetImage(uint32_t index)
    {
        return (Texture*)swapChainBuffers[index];
    }

    uint32_t VulkanSwapChain::GetCurrentBufferIndex() const
    {
        return currentBufferIndex;
    }

    uint32_t VulkanSwapChain::GetCurrentImageIndex() const
    {
        return acquireImageIndex;
    }

    size_t VulkanSwapChain::GetSwapChainBufferCount() const
    {
        return swapChainBufferNum;
    }

    CommandBuffer* VulkanSwapChain::GetCurrentCommandBuffer()
    {
        return bufferData[currentBufferIndex].MainCommandBuffer.get();
    }

    void VulkanSwapChain::SetVSync(bool vsync)
    {
        this->isVSync = vsync;
    }

    void VulkanSwapChain::AcquireNextImage()
    {
        static int FailedCount = 0;

        if(swapChainBufferNum == 1 && acquireImageIndex != (std::numeric_limits<uint32_t>::max)())
            return;

        {
            auto result = vkAcquireNextImageKHR(GET_DEVICE(), handle, UINT64_MAX, bufferData[currentBufferIndex].PresentSemaphore, VK_NULL_HANDLE, &acquireImageIndex);

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                if(result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    OnResize(width, height, true);
                }
            }
            else if(result != VK_SUCCESS)
            {
                FailedCount++;

                if(FailedCount > 10)
                {
                    gEngine->SetAppState(AppState::Closing);
                }

                return;
            }

            FailedCount = 0;
        }
    }

    void VulkanSwapChain::QueueSubmit()
    {
        auto& frameData = GetCurrentBufferData();
        frameData.MainCommandBuffer->Execute(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frameData.PresentSemaphore, true);
    }

    void VulkanSwapChain::Present(VkSemaphore semaphore)
    {
        VkPresentInfoKHR present;
        present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pNext              = VK_NULL_HANDLE;
        present.swapchainCount     = 1;
        present.pSwapchains        = &handle;
        present.pImageIndices      = &acquireImageIndex;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores    = &semaphore;
        present.pResults           = VK_NULL_HANDLE;

        auto error = vkQueuePresentKHR(gVulkanContext.GetDevice()->GetPresentQueue(), &present);

        if(error == VK_ERROR_OUT_OF_DATE_KHR)
        {
            LOG("[Vulkan] SwapChain out of date");
        }
        else if(error == VK_SUBOPTIMAL_KHR)
        {
            LOG("[Vulkan] SwapChain suboptimal");
        }
        else
        {
            LOG(error);
        }
    }

    void VulkanSwapChain::Begin()
    {
        currentBufferIndex = (currentBufferIndex + 1) % swapChainBufferNum;

        auto commandBuffer = GetCurrentBufferData().MainCommandBuffer;
        if(commandBuffer->GetState() == CommandBufferState::Submitted)
        {
            if(!commandBuffer->Wait())
            {
                return;
            }
        }
        commandBuffer->Reset();
        VulkanRenderer::GetDeletionQueue(currentBufferIndex).Flush();
        AcquireNextImage();

        commandBuffer->BeginRecording();
    }

    void VulkanSwapChain::End()
    {
        GetCurrentCommandBuffer()->EndRecording();
    }

    void VulkanSwapChain::OnResize(uint32_t _width, uint32_t _height, bool forceResize, Window* windowHandle)
    {
        if(!forceResize && width == _width && height == _height)
            return;

        gVulkanContext.WaitIdle();

        width  = _width;
        height = _height;

        oldHandle = handle;
        handle    = VK_NULL_HANDLE;

        //TODO: check
        if(windowHandle)
            Init(isVSync, windowHandle);
        else
        {
            Init(isVSync);
        }

        gVulkanContext.WaitIdle();
    }

} // NekoEngine