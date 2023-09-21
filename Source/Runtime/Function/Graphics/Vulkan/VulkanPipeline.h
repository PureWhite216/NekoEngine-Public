#pragma once
#include "Vk.h"
#include "RHI/Pipeline.h"
#include "VulkanFrameBuffer.h"
namespace NekoEngine
{

    class VulkanPipeline : public Pipeline
    {
    private:
        ArrayList<SharedPtr<VulkanFramebuffer>> frameBuffers;
        VkPipeline handle;
        VkPipelineLayout layout;

    public:
        VulkanPipeline(const PipelineDesc& pipelineDesc);
        ~VulkanPipeline();

        bool Init(const PipelineDesc& pipelineDesc);
        void Bind(CommandBuffer* commandBuffer, uint32_t layer) override;
        void End(CommandBuffer* commandBuffer) override;
        void ClearRenderTargets(CommandBuffer* commandBuffer) override;
        void TransitionAttachments();

        SharedPtr<Shader> GetShader() const override { return nullptr; }
        VkPipelineLayout& GetPipelineLayout() { return layout; }

        void CreateFramebuffers();

        static void MakeDefault();
    };

} // NekoEngine

