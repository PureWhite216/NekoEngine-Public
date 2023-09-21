#pragma once
#include "RHI/VertexBuffer.h"
#include "VulkanBuffer.h"
namespace NekoEngine
{

    class VulkanVertexBuffer : public VulkanBuffer, public VertexBuffer
    {
    public:
        VulkanVertexBuffer(const BufferUsage& usage);
        ~VulkanVertexBuffer();

        void Resize(uint32_t _size) override;
        void SetData(uint32_t _size, const void* data) override;
        void SetDataSub(uint32_t size, const void* data, uint32_t offset) override;
        void ReleasePointer() override;

        void Bind(CommandBuffer* commandBuffer, Pipeline* pipeline) override;
        void Unbind() override;

        uint32_t GetSize() override { return size; }
    protected:
        void* GetPointerInternal() override;

    };

} // NekoEngine

