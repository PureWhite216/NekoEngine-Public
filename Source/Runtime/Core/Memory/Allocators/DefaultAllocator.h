#pragma once

#include "Allocator.h"

namespace NekoEngine
{
    class DefaultAllocator : public Allocator
    {
    public:
        void* Malloc(size_t size, const char* file, int line) override;
        void Free(void* location) override;
    };

}
