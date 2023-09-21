//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <GLFW/glfw3.h>
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanCommandPool.h"
#include "RHI/Renderer.h"
#include "StringUtility.h"

namespace NekoEngine
{
    VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::findQueueFamilies(int flags)
    {
        QueueFamilyIndices indices;

        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if(flags & VK_QUEUE_COMPUTE_BIT)
        {
            for(uint32_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                auto& t_queueFamilyProperties = queueFamilyProperties[i];
                if((t_queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((t_queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    indices.computeFamily = i;
                    break;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if(flags & VK_QUEUE_TRANSFER_BIT)
        {
            for(uint32_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                auto& t_queueFamilyProperties = queueFamilyProperties[i];
                if((t_queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((t_queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((t_queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    indices.transferFamily = i;
                    break;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for(uint32_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if((flags & VK_QUEUE_TRANSFER_BIT) && indices.transferFamily == -1)
            {
                if(queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                    indices.transferFamily = i;
            }

            if((flags & VK_QUEUE_COMPUTE_BIT) && indices.computeFamily == -1)
            {
                if(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    indices.computeFamily = i;
            }

            if(flags & VK_QUEUE_GRAPHICS_BIT)
            {
                if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices.graphicsFamily = i;
            }
        }

        return indices;
    }
    VulkanPhysicalDevice::VulkanPhysicalDevice()
    {
        LOG("Initializing Vulkan physical device");
        auto vkInstance = gVulkanContext.GetVkInstance();
        vkEnumeratePhysicalDevices(vkInstance, &gpuNum, VK_NULL_HANDLE);
        if(gpuNum == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        ArrayList<VkPhysicalDevice> physicalDevices(gpuNum);
        vkEnumeratePhysicalDevices(vkInstance, &gpuNum, physicalDevices.data());
        handle = physicalDevices[0];

        //TODO: set gpu through project settings

        vkGetPhysicalDeviceProperties(handle, &properties);
        vkGetPhysicalDeviceFeatures(handle, &features);
        vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, nullptr);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, queueFamilyProperties.data());

        //TODO: set extensions
//        vkGetPhysicalDeviceProperties(handle, &properties);
//        vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);

        //TODO: check queue family support
        static const float defaultQueuePriority(0.0f);
        int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        queueFamilyIndices = findQueueFamilies(requestedQueueTypes);

        auto& caps                        = Renderer::capabilities;
//        caps.Vendor                       = deviceInfo.Vendor;
        caps.Renderer                     = std::string(properties.deviceName);
        caps.Version                      = StringUtility::ToString(properties.driverVersion);
        caps.MaxAnisotropy                = properties.limits.maxSamplerAnisotropy;
        caps.MaxSamples                   = properties.limits.maxSamplerAllocationCount;
        caps.MaxTextureUnits              = properties.limits.maxDescriptorSetSamplers;
        caps.UniformBufferOffsetAlignment = int(properties.limits.minUniformBufferOffsetAlignment);
        caps.SupportCompute               = false; // true; //Need to sort descriptor set management first

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(handle, nullptr, &extCount, nullptr);
        if(extCount > 0)
        {
            std::vector<VkExtensionProperties> tExtensions(extCount);
            if(vkEnumerateDeviceExtensionProperties(handle, nullptr, &extCount, &tExtensions.front()) == VK_SUCCESS)
            {
                for(const auto& ext : tExtensions)
                {
                    extensions.emplace(ext.extensionName);
                }
            }
        }

        {
            VkDeviceQueueCreateInfo queueInfo = {};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
        {
            if(queueFamilyIndices.computeFamily != queueFamilyIndices.graphicsFamily)
            {
                VkDeviceQueueCreateInfo queueInfo = {};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = queueFamilyIndices.computeFamily;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        {
            if(queueFamilyIndices.transferFamily != queueFamilyIndices.graphicsFamily && queueFamilyIndices.transferFamily != queueFamilyIndices.computeFamily)
            {
                VkDeviceQueueCreateInfo queueInfo = {};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                queueCreateInfos.push_back(queueInfo);
            }
        }
        LOG("Vulkan physical device initialized");
    }

    VulkanPhysicalDevice::~VulkanPhysicalDevice()
    {

    }

    VulkanDevice::VulkanDevice()
    {
    }

    bool VulkanDevice::Init()
    {
        LOG("Initializing Vulkan device");
        physicalDevice = MakeShared<VulkanPhysicalDevice>();

        VkPhysicalDeviceFeatures supportedFeatures;
        memset(&supportedFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
        memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
        vkGetPhysicalDeviceFeatures(physicalDevice->GetHandle(), &supportedFeatures);

        if(supportedFeatures.wideLines)
        {
            enabledFeatures.wideLines = VK_TRUE;
            Renderer::capabilities.WideLines = true;
        }
        else
        {
            Renderer::capabilities.WideLines = false;
        }

        if(supportedFeatures.samplerAnisotropy)
        {
            enabledFeatures.samplerAnisotropy = VK_TRUE;

        }

//        unsigned int glfwExtensionCount = 0;
//        const char** glfwExtensions;
//
//        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//        std::vector<const char*> deviceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        std::vector<const char*> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

//        deviceExtensions.push_back("VK_KHR_surface");
        if(physicalDevice->IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//            m_EnableDebugMarkers = true;
        }

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        indexingFeatures.runtimeDescriptorArray = VK_TRUE;
        indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

        // Create Device
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &indexingFeatures;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = physicalDevice->queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if(physicalDevice->GetHandle() == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        VkResult result = vkCreateDevice(physicalDevice->GetHandle(), &deviceCreateInfo, nullptr, &handle);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        LOG("Vulkan device initialized");

        vkGetDeviceQueue(handle, physicalDevice->queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(handle, physicalDevice->queueFamilyIndices.computeFamily, 0, &computeQueue);
        vkGetDeviceQueue(handle, physicalDevice->queueFamilyIndices.transferFamily, 0, &presentQueue);

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice->GetHandle();
        allocatorInfo.device = handle;
        allocatorInfo.instance = gVulkanContext.GetVkInstance();

        VmaVulkanFunctions fn;
        fn.vkAllocateMemory                        = (PFN_vkAllocateMemory)vkAllocateMemory;
        fn.vkBindBufferMemory                      = (PFN_vkBindBufferMemory)vkBindBufferMemory;
        fn.vkBindImageMemory                       = (PFN_vkBindImageMemory)vkBindImageMemory;
        fn.vkCmdCopyBuffer                         = (PFN_vkCmdCopyBuffer)vkCmdCopyBuffer;
        fn.vkCreateBuffer                          = (PFN_vkCreateBuffer)vkCreateBuffer;
        fn.vkCreateImage                           = (PFN_vkCreateImage)vkCreateImage;
        fn.vkDestroyBuffer                         = (PFN_vkDestroyBuffer)vkDestroyBuffer;
        fn.vkDestroyImage                          = (PFN_vkDestroyImage)vkDestroyImage;
        fn.vkFlushMappedMemoryRanges               = (PFN_vkFlushMappedMemoryRanges)vkFlushMappedMemoryRanges;
        fn.vkFreeMemory                            = (PFN_vkFreeMemory)vkFreeMemory;
        fn.vkGetBufferMemoryRequirements           = (PFN_vkGetBufferMemoryRequirements)vkGetBufferMemoryRequirements;
        fn.vkGetImageMemoryRequirements            = (PFN_vkGetImageMemoryRequirements)vkGetImageMemoryRequirements;
        fn.vkGetPhysicalDeviceMemoryProperties     = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetPhysicalDeviceMemoryProperties;
        fn.vkGetPhysicalDeviceProperties           = (PFN_vkGetPhysicalDeviceProperties)vkGetPhysicalDeviceProperties;
        fn.vkInvalidateMappedMemoryRanges          = (PFN_vkInvalidateMappedMemoryRanges)vkInvalidateMappedMemoryRanges;
        fn.vkMapMemory                             = (PFN_vkMapMemory)vkMapMemory;
        fn.vkUnmapMemory                           = (PFN_vkUnmapMemory)vkUnmapMemory;
        fn.vkGetBufferMemoryRequirements2KHR       = 0; //(PFN_vkGetBufferMemoryRequirements2KHR)vkGetBufferMemoryRequirements2KHR;
        fn.vkGetImageMemoryRequirements2KHR        = 0; //(PFN_vkGetImageMemoryRequirements2KHR)vkGetImageMemoryRequirements2KHR;
        fn.vkBindImageMemory2KHR                   = 0;
        fn.vkBindBufferMemory2KHR                  = 0;
        fn.vkGetPhysicalDeviceMemoryProperties2KHR = 0;
        fn.vkGetImageMemoryRequirements2KHR        = 0;
        fn.vkGetBufferMemoryRequirements2KHR       = 0;
        fn.vkGetInstanceProcAddr                   = (PFN_vkGetInstanceProcAddr)(vkGetInstanceProcAddr);
        fn.vkGetDeviceProcAddr                     = (PFN_vkGetDeviceProcAddr)vkGetDeviceProcAddr;
        allocatorInfo.pVulkanFunctions             = &fn;
        VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &allocator), "failed to create VMA allocator!");

        commandPool = MakeShared<VulkanCommandPool>(physicalDevice->queueFamilyIndices.graphicsFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        CreatePipelineCache();
        return VK_SUCCESS;
    }

    void VulkanDevice::CreatePipelineCache()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        if(vkCreatePipelineCache(handle, &pipelineCacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline cache!");
        }
    }

    VulkanDevice::~VulkanDevice()
    {
        commandPool.reset();
        vkDestroyPipelineCache(handle, pipelineCache, nullptr);
        vmaDestroyAllocator(allocator);
        vkDestroyDevice(handle, nullptr);
    }

    uint32_t VulkanPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags _properties) const
    {
        for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if((typeBits & 1) == 1)
            {
                if((memoryProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
                    return i;
            }
            typeBits >>= 1;
        }
    }

    bool VulkanPhysicalDevice::IsExtensionSupported(const std::string &extensionName) const
    {
        return extensions.find(extensionName) != extensions.end();
    }

    //TODO: Load Extension

} // NekoEngine