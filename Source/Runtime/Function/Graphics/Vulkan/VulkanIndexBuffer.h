#pragma once
#include "RHI/IndexBuffer.h"
#include "VulkanBuffer.h"

namespace NekoEngine
{

    class VulkanIndexBuffer : public IndexBuffer, public VulkanBuffer
    {
    public:
        VulkanIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage);
        VulkanIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage);
        ~VulkanIndexBuffer();

        void Bind(CommandBuffer* commandBuffer) const override;
        void Unbind() const override;
        uint32_t GetCount() const override;
        uint32_t GetSize() const override;
        void SetCount(uint32_t m_index_count) override { m_Count = m_index_count; };
        void ReleasePointer() override;

    };

} // NekoEngine

