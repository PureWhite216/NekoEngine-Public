//
// Created by 80529 on 2023/7/20.
//

#pragma once
#include "DescriptorSet.h"
namespace NekoEngine
{
    struct BufferElement
    {
        String name;
        RHIFormat format = RHIFormat::R32G32B32A32_Float;
        uint32_t offset = 0;
        bool isNormalised = false;
    };

    class BufferLayout
    {
    private:
        uint32_t size = 0;
        ArrayList<BufferElement> layout;
    public:
        BufferLayout() = default;

        inline ArrayList<BufferElement>& GetLayout() { return layout; }
        inline uint32_t GetStride() { return size; }

        void Push(const String& _name, RHIFormat _format, bool _isNormalised);
    };


} // NekoEngine

