#pragma once
#include "CommandBuffer.h"

namespace NekoEngine
{

    class ImGuiRenderer
    {
    public:
        virtual ~ImGuiRenderer()                               = default;
        virtual void Init()                                    = 0;
        virtual void Render(CommandBuffer* commandBuffer)      = 0;
        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void Clear() { }
        virtual bool Implemented() const  = 0;
        virtual void RebuildFontTexture() = 0;
    };

} // NekoEngine

