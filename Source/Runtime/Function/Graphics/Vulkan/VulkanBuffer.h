#pragma once
#include "Vk.h"
namespace NekoEngine
{

    class VulkanBuffer
    {
    protected:
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDeviceSize size = 0;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags property;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkDescriptorBufferInfo descriptor;
        void* mapped = nullptr;

        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocation mappedAllocation = VK_NULL_HANDLE;

    public:
        VulkanBuffer() = default;
        VulkanBuffer(VkBufferUsageFlags _usage, VkMemoryPropertyFlags _property, uint32_t size, const void* data);
        virtual ~VulkanBuffer();

        void Init(VkBufferUsageFlags _usage, VkMemoryPropertyFlags _property, uint32_t size, const void* data);
        void SetData(const void* data, uint32_t size, uint32_t offset = 0);
        void Resize(uint32_t size, const void* data = nullptr);
        void FreeResources(bool deletionQueue = false);

        void Map();
        void Unmap();
        void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void SetUsage(VkBufferUsageFlags flags) { usageFlags = flags; }
        void SetMemoryProperyFlags(VkBufferUsageFlags flags) { usageFlags = flags; }

        const VkBuffer& GetHandle() const { return buffer; }
        const VkBuffer& GetBuffer() const { return buffer; }
        const VkDeviceMemory& GetMemory() const { return memory; }
        const VkDescriptorBufferInfo& GetDescriptor() const { return descriptor; }
        const VkDeviceSize& GetSize() const { return size; }
        const void* GetMappedPointer() const { return mapped; }
        const VmaAllocation& GetAllocation() const { return allocation; }

    };

} // NekoEngine
