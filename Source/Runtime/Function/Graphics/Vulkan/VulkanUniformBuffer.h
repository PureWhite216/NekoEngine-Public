#pragma once
#include "RHI/UniformBuffer.h"
#include "VulkanBuffer.h"

namespace NekoEngine
{

    class VulkanUniformBuffer : public VulkanBuffer, public UniformBuffer
    {
    public:
        VulkanUniformBuffer(uint32_t size, const void* data);
        VulkanUniformBuffer() = default;
        ~VulkanUniformBuffer() = default;

        void Init(uint32_t size, const void* data) override;

        void SetData(uint32_t size, const void* data) override;
        void SetData(const void* data) override { SetData(size, data); }
        void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) override;
        uint8_t* GetBuffer() const override;
        VkBuffer GetBuffer() { return buffer; }
    };

} // NekoEngine

