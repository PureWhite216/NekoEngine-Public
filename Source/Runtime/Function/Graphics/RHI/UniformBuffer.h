
#pragma once
#include "Core.h"
namespace NekoEngine
{

    class UniformBuffer
    {
    public:
        virtual ~UniformBuffer() = default;
        virtual void Init(uint32_t size, const void* data)                              = 0;
        virtual void SetData(const void* data)                                          = 0;
        virtual void SetData(uint32_t size, const void* data)                           = 0;
        virtual void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) = 0;
        virtual uint8_t* GetBuffer() const = 0;
    };

} // NekoEngine
