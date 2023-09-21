#include "VulkanIndexBuffer.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

namespace NekoEngine
{
    VulkanIndexBuffer::VulkanIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage)
    {
        m_Size = count * sizeof(uint16_t);
        m_Count = count;
        m_Usage = bufferUsage;
    }

    VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage)
    {
        m_Size = count * sizeof(uint32_t);
        m_Count = count;
        m_Usage = bufferUsage;
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        if(m_MappedBuffer)
        {
            VulkanBuffer::Flush(m_Size);
            VulkanBuffer::Unmap();
            m_MappedBuffer = false;
        }
    }

    void VulkanIndexBuffer::Bind(CommandBuffer* commandBuffer) const
    {
        vkCmdBindIndexBuffer(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanIndexBuffer::Unbind() const
    {

    }

    uint32_t VulkanIndexBuffer::GetCount() const
    {
        return m_Count;
    }

    uint32_t VulkanIndexBuffer::GetSize() const
    {
        return m_Size;
    }

    void VulkanIndexBuffer::ReleasePointer()
    {
        if(m_MappedBuffer)
        {
            VulkanBuffer::Flush(m_Size);
            VulkanBuffer::Unmap();
            m_MappedBuffer = false;
        }
    }
} // NekoEngine