#include "VulkanVertexBuffer.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

namespace NekoEngine
{
    VulkanVertexBuffer::VulkanVertexBuffer(const BufferUsage &usage)
    {
        bufferUsage = usage;
        size = 0;
        isMapped = false;

        SetUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        SetMemoryProperyFlags(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    }

    void VulkanVertexBuffer::Resize(uint32_t _size)
    {
        if(_size == 0)
        {
            return;
        }
        if(_size != this->size)
        {
            this->size = _size;
            VulkanBuffer::Resize(_size);
        }
    }

    void* VulkanVertexBuffer::GetPointerInternal()
    {
        if(!isMapped)
        {
            VulkanBuffer::Map();
            isMapped = true;
        }

        return mapped;
    }

    void VulkanVertexBuffer::SetData(uint32_t _size, const void *data)
    {
        if(_size == 0)
        {
            return;
        }
        if(_size > size)
        {
            size = _size;
            VulkanBuffer::Resize(_size);
        }
        else
        {
            VulkanBuffer::SetData(data, _size);
        }
    }

    void VulkanVertexBuffer::SetDataSub(uint32_t _size, const void* data, uint32_t offset)
    {
        //TODO: Need Check
        if(_size > size)
        {
            size = _size;
            VulkanBuffer::Resize(_size);
        }
        else
        {
            VulkanBuffer::SetData(data, _size);
        }
    }

    void VulkanVertexBuffer::Bind(CommandBuffer* commandBuffer, Pipeline* pipeline)
    {
//        VkDeviceSize offset = 0;
        if(commandBuffer)
        {
            vkCmdBindVertexBuffers(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), 0, 1, &(VulkanBuffer::buffer), 0);
        }
    }

    void VulkanVertexBuffer::Unbind()
    {
    }

    void VulkanVertexBuffer::ReleasePointer()
    {
        if(mapped)
        {
            Flush(size);
            Unmap();
            isMapped = false;
        }
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        if(isMapped)
        {
            Flush(size);
            Unmap();
            isMapped = false;
        }
    }


} // NekoEngine