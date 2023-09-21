#include "VulkanDevice.h"
#include "VulkanUniformBuffer.h"

namespace NekoEngine
{

    VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, const void* data)
    {
        Init(size, data);
    }

    void VulkanUniformBuffer::Init(uint32_t size, const void* data)
    {
        VulkanBuffer::Init(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size, data);
    }

    void VulkanUniformBuffer::SetData(uint32_t size, const void* data)
    {
        Map();
        memcpy(mapped, data, size);
        Unmap();
    }

    void VulkanUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void* data)
    {
        Map();
        memcpy(mapped, data, size);
        Flush(size);
        Unmap();
    }

    uint8_t* VulkanUniformBuffer::GetBuffer() const
    {
        return nullptr;
    }


} // NekoEngine