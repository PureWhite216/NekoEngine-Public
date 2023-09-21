#pragma once
#include "Core.h"
#include "RHI/Framebuffer.h"
namespace NekoEngine
{

    class RenderPass;
    class Pipeline;

    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer() = default;

        virtual bool Init(bool isPrimary = true) = 0;
        virtual void Unload() = 0;
        virtual void BeginRecording() = 0;
        virtual void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) = 0;
        virtual void EndRecording() = 0;
        virtual void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) = 0;
        virtual void UpdateViewport(uint32_t width, uint32_t height, bool flipViewport = false) = 0;
        virtual void BindPipeline(Pipeline* pipeline) = 0;
        virtual void UnBindPipeline() = 0;
        virtual bool Flush(){ return true; }
        virtual void Submit(){}
    };

} // NekoEngine

