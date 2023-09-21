//
// Created by 80529 on 2023/7/19.
//

#include "Texture.h"

namespace NekoEngine
{
    RHIFormat Texture::BitsToFormat(uint32_t bits)
    {
        switch(bits)
        {
            case 8:
                return RHIFormat::R8_Unorm;
            case 16:
                return RHIFormat::R8G8_Unorm;
            case 24:
                return RHIFormat::R8G8B8_Unorm;
            case 32:
                return RHIFormat::R8G8B8A8_Unorm;
            case 48:
                return RHIFormat::R16G16B16_Float;
            case 64:
                return RHIFormat::R16G16B16A16_Float;
            case 96:
                return RHIFormat::R32G32B32_Float;
            case 128:
                return RHIFormat::R32G32B32A32_Float;
            default:
                return RHIFormat::R8G8B8A8_Unorm;
        }
    }

    uint32_t Texture::GetBitsFromFormat(const RHIFormat format)
    {
        switch(format)
        {
            case RHIFormat::R8_Unorm:
                return 8;
            case RHIFormat::D16_Unorm:
                return 16;
            case RHIFormat::R8G8_Unorm:
                return 16;
            case RHIFormat::R8G8B8_Unorm:
                return 24;
            case RHIFormat::R16G16B16_Float:
                return 48;
            case RHIFormat::R32G32B32_Float:
                return 96;
            case RHIFormat::R8G8B8A8_Unorm:
                return 32;
            case RHIFormat::R16G16B16A16_Float:
                return 64;
            case RHIFormat::R32G32B32A32_Float:
                return 128;
            default:
                return 32;
        }
    }

    uint32_t Texture::BitsToChannelCount(uint32_t bits)
    {
        switch(bits)
        {
            case 8:
                return 1;
            case 16:
                return 2;
            case 24:
                return 3;
            case 32:
                return 4;
            case 48:
                return 3;
            case 64:
                return 4;
            case 96:
                return 3;
            case 128:
                return 4;
            default:
                ASSERT(false, "[Texture] Unsupported image bit-depth!");
                return 4;
        }
    }
} // NekoEngine