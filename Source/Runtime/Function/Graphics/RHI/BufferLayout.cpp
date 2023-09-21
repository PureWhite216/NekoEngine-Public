#include "BufferLayout.h"

namespace NekoEngine
{
    void BufferLayout::Push(const String& _name, RHIFormat _format, bool _isNormalised)
    {
        uint32_t _size;
        switch(_format) //TODO other format
        {
            case RHIFormat::R32_UInt:
                _size = sizeof(uint32_t);
                break;
            case RHIFormat::R8_UInt:
                _size = sizeof(uint8_t);
                break;
            case RHIFormat::R32_Float:
                _size = sizeof(float);
                break;
            case RHIFormat::R32G32_Float:
                _size = sizeof(FVector2);
                break;
            case RHIFormat::R32G32B32_Float:
                _size = sizeof(FVector3);
                break;
            case RHIFormat::R32G32B32A32_Float:
                _size = sizeof(FVector4);
                break;
            default:
                return;
        }
        layout.push_back({_name, _format, _size, _isNormalised});
        size += _size;
    }
} // NekoEngine