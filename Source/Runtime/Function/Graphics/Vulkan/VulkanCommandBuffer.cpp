#include "VulkanFramebuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "VulkanInitializer.h"

namespace NekoEngine
{
    VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool _commandPool) : commandPool(_commandPool), isPrimary(true)
    {}

    bool VulkanCommandBuffer::Init(bool _isPrimary)
    {
        isPrimary = _isPrimary;
        commandPool = gVulkanContext.GetDevice()->GetCommandPool()->GetHandle();

        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = commandPool;
        info.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        info.commandBufferCount = 1;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(GET_DEVICE(), &info, &handle), "failed to allocate command buffer!");

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext = nullptr;
        VK_CHECK_RESULT(vkCreateSemaphore(GET_DEVICE(), &semaphoreInfo, nullptr, &semaphore), "failed to create semaphore!");

        fence = MakeShared<VulkanFence>(true);

        return  true;
    }

    bool VulkanCommandBuffer::Init(bool _isPrimary, VkCommandPool _commandPool)
    {
        isPrimary = _isPrimary;
        commandPool = _commandPool;

        VkCommandBufferAllocateInfo cmdBufferCreateInfo = VKInitialisers::CommandBufferAllocateInfo(commandPool, isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, 1);

        VK_CHECK_RESULT(vkAllocateCommandBuffers(GET_DEVICE(), &cmdBufferCreateInfo, &handle), "failed to allocate command buffer!");

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext                 = nullptr;

        VK_CHECK_RESULT(vkCreateSemaphore(GET_DEVICE(), &semaphoreInfo, nullptr, &semaphore), "failed to create semaphore!");
        fence = MakeShared<VulkanFence>(true);

        return true;
    }

    void VulkanCommandBuffer::BeginRecording()
    {
        if(!isPrimary)
        {
            RUNTIME_ERROR("Recording secondary command buffer!");
        }

        state = CommandBufferState::Recording;
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK_RESULT(vkBeginCommandBuffer(handle, &info), "failed to begin command buffer!");
    }

    void VulkanCommandBuffer::BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer)
    {
        if(isPrimary)
        {
            RUNTIME_ERROR("Recording primary command buffer!");
        }
        state = CommandBufferState::Recording;

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = dynamic_cast<VulkanRenderPass*>(renderPass)->GetHandle();
        inheritanceInfo.framebuffer = dynamic_cast<VulkanFramebuffer*>(framebuffer)->GetHandle();

        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        info.pInheritanceInfo = &inheritanceInfo;
        VK_CHECK_RESULT(vkBeginCommandBuffer(handle, &info), "failed to begin command buffer!");
    }

    void VulkanCommandBuffer::BindPipeline(Pipeline* pipeline)
    {
        if(pipeline != boundPipeline.get())
        {
            if(boundPipeline)
                boundPipeline->End(this);

            pipeline->Bind(this);
            boundPipeline = SharedPtr<Pipeline>(pipeline);

        }
    }

    void VulkanCommandBuffer::UnBindPipeline()
    {
        if(boundPipeline)
            boundPipeline->End(this);
        boundPipeline.reset();
    }

    void VulkanCommandBuffer::UpdateViewport(uint32_t width, uint32_t height, bool flipViewport)
    {
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = flipViewport ? static_cast<float>(height) : 0.0f;
        viewport.width = static_cast<float>(width);
        viewport.height = -static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        if(flipViewport)
        {
            viewport.y = static_cast<float>(height);
            viewport.height = -static_cast<float>(height);
        }

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {width, height};

        vkCmdSetViewport(handle, 0, 1, &viewport);
        vkCmdSetScissor(handle, 0, 1, &scissor);
    }

    void VulkanCommandBuffer::Reset()
    {
        VK_CHECK_RESULT(vkResetCommandBuffer(handle, 0), "failed to reset command buffer!");
    }

    bool VulkanCommandBuffer::Flush()
    {
        if(state == CommandBufferState::Idle) return true;

        VulkanUtility::WaitIdle();

        if(state == CommandBufferState::Submitted) return Wait();

        return true;
    }

    bool VulkanCommandBuffer::Wait()
    {
        if(state == CommandBufferState::Submitted)
        {
            LOG("Command buffer already submitted!");
            return false;
        }

        fence->WaitAndReset();
        state = CommandBufferState::Idle;

        return true;
    }

    void VulkanCommandBuffer::Submit()
    {
    }

    void VulkanCommandBuffer::Execute(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, bool waitFence)
    {
        if(!isPrimary)
        {
            RUNTIME_ERROR("Executing secondary command buffer!");
        }
        if(state != CommandBufferState::Ended)
        {
            RUNTIME_ERROR("CommandBuffer executed before ended recording!");
        }

        uint32_t waitSemaphoreCount = waitSemaphore ? 1 : 0;
        uint32_t signalSemaphoreCount = semaphore ? 1 : 0;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &flags;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = &semaphore;

        fence->Reset();

        VK_CHECK_RESULT(vkQueueSubmit(gVulkanContext.GetDevice()->GetGraphicsQueue(), 1, &submitInfo, fence->GetHandle()), "failed to submit command buffer!");

        state = CommandBufferState::Submitted;
    }

    void VulkanCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
    {
        if(isPrimary)
        {
            RUNTIME_ERROR("Executing primary command buffer!");
        }
        state = CommandBufferState::Submitted;
        vkCmdExecuteCommands(dynamic_cast<VulkanCommandBuffer*>(primaryCmdBuffer)->GetHandle(), 1, &handle);
    }

    VulkanCommandBuffer::~VulkanCommandBuffer()
    {
        VulkanCommandBuffer::Unload();
    }

    void VulkanCommandBuffer::Unload()
    {
        VulkanUtility::WaitIdle();

        if(state == CommandBufferState::Submitted)
            Wait();

        fence = nullptr;
        vkDestroySemaphore(GET_DEVICE(), semaphore, nullptr);
        vkFreeCommandBuffers(GET_DEVICE(), commandPool, 1, &handle);
    }

    void VulkanCommandBuffer::EndRecording()
    {
        if(boundPipeline)
            boundPipeline->End(this);

        boundPipeline = nullptr;

        VK_CHECK_RESULT(vkEndCommandBuffer(handle), "failed to end command buffer!");
        state = CommandBufferState::Ended;
    }
} // NekoEngine