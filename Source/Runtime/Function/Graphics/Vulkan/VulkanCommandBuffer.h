#pragma once

#include "Vk.h"
#include "RHI/Framebuffer.h"
#include "RHI/CommandBuffer.h"
#include "VulkanFence.h"

namespace NekoEngine
{
    enum class CommandBufferState : uint8_t
    {
        Idle,
        Recording,
        Ended,
        Submitted
    };

    class RenderPass;

    class Pipeline;

    class VulkanCommandBuffer : public CommandBuffer
    {
    private:
        CommandBufferState state = CommandBufferState::Idle;
        VkCommandBuffer handle = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        SharedPtr<VulkanFence> fence;
        VkSemaphore semaphore = VK_NULL_HANDLE;
        SharedPtr<Pipeline> boundPipeline;
        SharedPtr<RenderPass> boundRenderPass;
        bool isPrimary = false;

    public:
        VulkanCommandBuffer() = default;
        VulkanCommandBuffer(VkCommandPool _commandPool);
        ~VulkanCommandBuffer();

        bool Init(bool _isPrimary) override;
        void Unload() override;
        void BeginRecording() override;
        void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) override;
        void EndRecording() override;
        void Reset();
        bool Flush() override;
        bool Wait();
        void Submit() override;
        void UpdateViewport(uint32_t width, uint32_t height, bool flipViewport) override;

        void Execute(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, bool waitFence);
        void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;

        void BindPipeline(Pipeline* pipeline) override;
        void UnBindPipeline() override;

    public:

        VkCommandBuffer GetHandle() const { return handle; }
        CommandBufferState GetState() const { return state; }

        const VkSemaphore GetSemaphore() const { return semaphore; }
    };

} // NekoEngine

