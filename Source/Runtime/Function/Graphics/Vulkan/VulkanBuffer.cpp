#include "VulkanBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    VulkanBuffer::VulkanBuffer(VkBufferUsageFlags _usage, VkMemoryPropertyFlags _property, uint32_t size, const void* data)
    {
        Init(_usage, _property, size, data);
    }

    void VulkanBuffer::Init(VkBufferUsageFlags _usage, VkMemoryPropertyFlags _property, uint32_t size, const void* data)
    {
        usageFlags = _usage;
        property = _property;
        this->size = size;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        bool isMappable = (_property & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;
        allocInfo.flags = isMappable ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
        allocInfo.preferredFlags = _property;
        vmaCreateBuffer(gVulkanContext.GetDevice()->GetAllocator(), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

        if(data != nullptr)
        {
            SetData(data, size);
        }
    }

    void VulkanBuffer::SetData(const void* data, uint32_t _size, uint32_t offset)
    {
        Map();
        memcpy(mapped, data, _size);
        Unmap();
    }

    void VulkanBuffer::Resize(uint32_t _size, const void* data)
    {
        FreeResources(true);
        Init(usageFlags, property, _size, data);
    }

    void VulkanBuffer::Flush(VkDeviceSize _size, VkDeviceSize offset)
    {
        vmaFlushAllocation(gVulkanContext.GetDevice()->GetAllocator(), allocation, offset, _size);
    }

    void VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        vmaInvalidateAllocation(gVulkanContext.GetDevice()->GetAllocator(), allocation, offset, size);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        FreeResources(true);
    }

    void VulkanBuffer::FreeResources(bool useDeletionQue)
    {
        if(buffer != VK_NULL_HANDLE)
        {
            if(useDeletionQue)
            {
                DeletionQueue deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

                auto tBuffer = this->buffer;
                auto tAlloc = this->allocation;

                deletionQueue.Push([tBuffer, tAlloc]()
                                   {
                                       vmaDestroyBuffer(gVulkanContext.GetDevice()->GetAllocator(), tBuffer, tAlloc);
                                   });
            }
            else
            {
                vmaDestroyBuffer(gVulkanContext.GetDevice()->GetAllocator(), buffer, allocation);
            }
        }
        buffer = VK_NULL_HANDLE;
    }

    void VulkanBuffer::Map()
    {
        VK_CHECK_RESULT(vmaMapMemory(gVulkanContext.GetDevice()->GetAllocator(), allocation, &mapped), "failed to map buffer memory!");
    }

    void VulkanBuffer::Unmap()
    {
        if(mapped != nullptr)
        {
            vmaUnmapMemory(gVulkanContext.GetDevice()->GetAllocator(), allocation);
            mapped = nullptr;
        }
    }


} // NekoEngine