#pragma once
#include "Core.h"
namespace NekoEngine
{
    namespace ImageLoader
    {
        uint8_t* LoadImageFromFile(const char* filename, uint32_t* width = nullptr, uint32_t* height = nullptr,
                                   uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false,
                                   bool srgb = true);

        uint8_t* LoadImageFromFile(const std::string &filename, uint32_t* width = nullptr, uint32_t* height = nullptr,
                          uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false, bool srgb = true);
    }
} // NekoEngine