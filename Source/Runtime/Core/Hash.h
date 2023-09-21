#pragma once
#include "Core.h"
namespace NekoEngine
{

    inline void HashCombine(uint64_t& seed) { }

    template <typename T, typename... Rest>
    inline void HashCombine(uint64_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        HashCombine(seed, rest...);
    }

} // NekoEngine

