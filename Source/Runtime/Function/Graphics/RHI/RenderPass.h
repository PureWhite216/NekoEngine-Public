#pragma once
#include "Definitions.h"
namespace NekoEngine
{

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void BeginRenderpass(CommandBuffer* commandBuffer, Color clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const = 0;
        virtual void EndRenderpass(CommandBuffer* commandBuffer) = 0;
        virtual int GetAttachmentCount() const = 0;

        static void ClearCache();
        static void DeleteUnusedCache();

        static SharedPtr<RenderPass> Get(const RenderPassDesc& renderPassDesc);
    };

} // NekoEngine