//
// Created by 80529 on 2023/7/18.
//

#pragma once
// TODO Note: may redefine if include
//#include "Window/Window.h"
#include "Texture.h"
#include "CommandBuffer.h"

namespace NekoEngine
{
    class Window;

    class SwapChain
    {
    public:
        SwapChain() = default;
        virtual ~SwapChain() = default;
//        virtual bool Init(bool vsync) = 0;
        virtual bool Init(bool vsync, Window* window) = 0;
        virtual bool Init(bool vsync) = 0;
        virtual Texture* GetCurrentImage() = 0;
        virtual Texture* GetImage(uint32_t index) = 0;
        virtual uint32_t GetCurrentBufferIndex() const = 0;
        virtual uint32_t GetCurrentImageIndex() const    = 0;
        virtual size_t GetSwapChainBufferCount() const   = 0;
        virtual CommandBuffer* GetCurrentCommandBuffer() = 0;
        virtual void SetVSync(bool vsync)                = 0;
    };
}