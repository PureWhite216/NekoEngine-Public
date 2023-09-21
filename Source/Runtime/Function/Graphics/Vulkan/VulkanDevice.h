#pragma once
#include "Vk.h"
#include "VulkanCommandPool.h"
namespace NekoEngine
{
    class VulkanPhysicalDevice
    {
    private:
        friend class VulkanDevice;

        struct QueueFamilyIndices
        {
            int graphicsFamily = -1;
            int computeFamily = -1;
            int transferFamily = -1;

            bool isComplete()
            {
                return graphicsFamily >= 0 && computeFamily >= 0 && transferFamily >= 0;
            }
        };
        QueueFamilyIndices findQueueFamilies(int flags);

    private:
        uint32_t gpuNum = 0;
        VkPhysicalDevice handle;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        QueueFamilyIndices queueFamilyIndices;

        ArrayList<VkQueueFamilyProperties> queueFamilyProperties;
        ArrayList<VkDeviceQueueCreateInfo> queueCreateInfos;
        HashSet<String> extensions;

    public:
        VulkanPhysicalDevice();
        ~VulkanPhysicalDevice();

        bool IsExtensionSupported(const std::string& extensionName) const;
        uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

        VkPhysicalDevice GetHandle() const { return handle; }
        VkPhysicalDeviceProperties GetProperties() const { return properties; }
        VkPhysicalDeviceFeatures GetFeatures() const { return features; }
        VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return memoryProperties; }
        const ArrayList<VkQueueFamilyProperties>& GetQueueFamilyProperties() const { return queueFamilyProperties; }

        inline int32_t GetGraphicsFamilyIndex() const { return queueFamilyIndices.graphicsFamily; }

        uint32_t GetGPUCount() const
        {
            return gpuNum;
        }

    };

    class VulkanDevice
    {
    private:
        VkDevice handle;
        SharedPtr<VulkanPhysicalDevice> physicalDevice;

        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkQueue computeQueue;
//        VkQueue transferQueue;

        SharedPtr<VulkanCommandPool> commandPool;
//        VkCommandPool computeCommandPool;
//        VkCommandPool transferCommandPool;

        VkPipelineCache pipelineCache;
        VkDescriptorPool descriptorPool;
        VkPhysicalDeviceFeatures enabledFeatures;

        VmaAllocator allocator;

    public:
        VulkanDevice();
        ~VulkanDevice();

        VkDevice GetHandle() const { return handle; }
        SharedPtr<VulkanPhysicalDevice> GetPhysicalDevice() const { return physicalDevice; }

        bool Init();
        void CreatePipelineCache();
        SharedPtr<VulkanCommandPool> GetCommandPool(){ return commandPool; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }

        VkPipelineCache GetPipelineCache() const
        {
            return pipelineCache;
        }

        VmaAllocator& GetAllocator(){ return allocator; }

        VkQueue GetComputeQueue() const { return computeQueue; }
        VkQueue GetPresentQueue() const { return presentQueue; }

    };

} // NekoEngine
