//
// Created by 80529 on 2023/7/20.
//

#pragma once
#include "Definitions.h"
namespace NekoEngine
{
    class IndexBuffer
    {
    protected:
        BufferUsage m_Usage;
        uint32_t m_Count;
        uint32_t m_Size;
        bool m_MappedBuffer = false;
    public:
        virtual ~IndexBuffer()                                          = default;
        virtual void Bind(CommandBuffer* commandBuffer = nullptr) const = 0;
        virtual void Unbind() const                                     = 0;

        virtual uint32_t GetCount() const = 0;
        virtual uint32_t GetSize() const { return 0; }
        virtual void SetCount(uint32_t m_index_count) = 0;
        virtual void ReleasePointer() {};
    };

} // NekoEngine