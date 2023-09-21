#pragma once
#include "Renderer.h"
namespace NekoEngine
{

    enum class RENDER_API : uint8_t
    {
        VULKAN
    };

    class GraphicsContext
    {
        friend class VulkanRenderer;
    private:
        SharedPtr<Renderer> renderer;
    public:
        GraphicsContext()= default;
        virtual ~GraphicsContext() = default;

        //TODO Set Rendering API

        virtual void Init()               = 0;
        virtual void Present()            = 0;
//        virtual float GetGPUMemoryUsed()  = 0;
//        virtual float GetTotalGPUMemory() = 0;

        virtual size_t GetMinUniformBufferOffsetAlignment() const = 0;
        virtual bool FlipImGUITexture() const                     = 0;
        virtual void WaitIdle() const                             = 0;
        virtual void OnImGui()                                    = 0;

        static RENDER_API GetRenderAPI() { return RENDER_API::VULKAN; };

        SharedPtr<Renderer> GetRenderer() const
        {
            if(renderer == nullptr)
                throw std::runtime_error("Renderer is nullptr");
            return renderer;
        }
    };

} // NekoEngine
