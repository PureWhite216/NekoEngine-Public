#include "VulkanTexture.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"
#include "VulkanRenderer.h"

namespace NekoEngine
{
    VulkanRenderPass::VulkanRenderPass(const RenderPassDesc &renderPassDesc)
    {
        Init(renderPassDesc);
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
        delete[] clearValues;

        DeletionQueue &deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();
        VkRenderPass renderPass = handle;

        deletionQueue.Push([renderPass]
                           { vkDestroyRenderPass(GET_DEVICE(), renderPass, VK_NULL_HANDLE); });
    }

    bool VulkanRenderPass::Init(const RenderPassDesc &renderPassDesc)
    {
        ArrayList<VkAttachmentDescription> attachments;
        ArrayList<VkAttachmentReference> colorAttachmentRefs;
        ArrayList<VkAttachmentReference> depthAttachmentRefs;

        isDepthOnly = true;
        isClearDepth = false;

        //TODO support cube depth
        for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
        {
            attachments.push_back(
                    GetAttachmentDescription(renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i],
                                             renderPassDesc.clear));

            if(renderPassDesc.attachmentTypes[i] == TextureType::COLOUR)
            {
                VkImageLayout layout = dynamic_cast<VulkanTexture2D*>(renderPassDesc.attachments[i])->GetImageLayout();
                VkAttachmentReference colourAttachmentRef = {};
                colourAttachmentRef.attachment = uint32_t(i);
                colourAttachmentRef.layout =
                        layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                colorAttachmentRefs.push_back(colourAttachmentRef);
                isDepthOnly = false;
            }
//            else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH)
//            {
//                VkAttachmentReference depthAttachmentRef = {};
//                depthAttachmentRef.attachment = uint32_t(i);
//                depthAttachmentRef.layout = ((VulkanTextureDepth*) renderPassDesc.attachments[i])->GetImageLayout();
//                depthAttachmentReferences.push_back(depthAttachmentRef);
//                m_ClearDepth = renderPassDesc.clear;
//            }
//            else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
//            {
//                VkAttachmentReference depthAttachmentRef = {};
//                depthAttachmentRef.attachment = uint32_t(i);
//                depthAttachmentRef.layout = ((VulkanTextureDepthArray*) renderPassDesc.attachments[i])->GetImageLayout();
//                depthAttachmentReferences.push_back(depthAttachmentRef);
//                m_ClearDepth = renderPassDesc.clear;
//            }
//            else if(renderPassDesc.attachmentTypes[i] == TextureType::CUBE)
//            {
//                VkImageLayout layout = ((VulkanTextureCube*) renderPassDesc.attachments[i])->GetImageLayout();
//                VkAttachmentReference colourAttachmentRef = {};
//                colourAttachmentRef.attachment = uint32_t(i);
//                colourAttachmentRef.layout =
//                        layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
//                colourAttachmentReferences.push_back(colourAttachmentRef);
//                m_DepthOnly = false;
//            }
            else
            {
                LOG("Unsupported texture attachment");
            }
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = colorAttachmentRefs.size();
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = depthAttachmentRefs.data();

        colorAttachmentCount = colorAttachmentRefs.size();

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;

        VK_CHECK_RESULT(vkCreateRenderPass(GET_DEVICE(), &renderPassInfo, nullptr, &handle),
                        "failed to create render pass!");

        clearValueCount = renderPassDesc.attachmentCount;
        clearValues = new VkClearValue[clearValueCount];
        isSwapChainTarget = renderPassDesc.swapchainTarget;

        return true;
    }

    void VulkanRenderPass::BeginRenderpass(CommandBuffer* commandBuffer, Color clearColour, Framebuffer* frame,
                                           SubPassContents contents, uint32_t width, uint32_t height) const
    {
        if(!isDepthOnly)
        {
            for(int i = 0; i < clearValueCount; i++)
            {
                clearValues[i].color.float32[0] = clearColour[0];
                clearValues[i].color.float32[1] = clearColour[1];
                clearValues[i].color.float32[2] = clearColour[2];
                clearValues[i].color.float32[3] = clearColour[3];
            }
        }

        if(isClearDepth)
        {
            clearValues[clearValueCount - 1].depthStencil.depth = 1.0f;
            clearValues[clearValueCount - 1].depthStencil.stencil = 0;
        }

        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = handle;
        info.framebuffer = dynamic_cast<VulkanFramebuffer*>(frame)->GetHandle();
        info.renderArea = {{0,     0},
                           {width, height}};
        info.clearValueCount = clearValueCount;
        info.pClearValues = clearValues;

        vkCmdBeginRenderPass(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), &info,
                             SubPassContents2Vk(contents));
        commandBuffer->UpdateViewport(width, height, isSwapChainTarget);
    }

    void VulkanRenderPass::EndRenderpass(CommandBuffer* commandBuffer)
    {
        vkCmdEndRenderPass(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle());
    }


    VkAttachmentDescription GetAttachmentDescription(TextureType type, Texture* texture, bool isClear)
    {
        //TODO: Cube Depth
        VkAttachmentDescription attachment = {};
        if(type == TextureType::COLOUR)
        {
            VulkanTexture2D* colourTexture = dynamic_cast<VulkanTexture2D*>(texture);
            attachment.format = colourTexture->GetVkFormat();
            attachment.initialLayout = colourTexture->GetImageLayout();
            attachment.finalLayout = attachment.initialLayout;
        }
//        else if(type == TextureType::CUBE)
//        {
//            VulkanTextureCube* colourTexture = ((VulkanTextureCube*)texture);
//            attachment.format            = colourTexture->GetVKFormat();
//            attachment.initialLayout     = colourTexture->GetImageLayout();
//            attachment.finalLayout       = attachment.initialLayout;
//        }
//        else if(type == TextureType::DEPTH)
//        {
//            attachment.format        = ((VulkanTextureDepth*)texture)->GetVKFormat();
//            attachment.initialLayout = ((VulkanTextureDepth*)texture)->GetImageLayout();
//            attachment.finalLayout   = attachment.initialLayout;
//        }
//        else if(type == TextureType::DEPTHARRAY)
//        {
//            attachment.format        = ((VulkanTextureDepthArray*)texture)->GetVKFormat();
//            attachment.initialLayout = ((VulkanTextureDepthArray*)texture)->GetImageLayout();
//            attachment.finalLayout   = attachment.initialLayout;
//        }
        else
        {
            LOG("Invalid texture type!");
            return attachment;
        }

        if(isClear)
        {
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
        else
        {
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }

        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.flags = 0;

        return attachment;
    }

    VkSubpassContents SubPassContents2Vk(SubPassContents contents)
    {
        switch(contents)
        {
            case INLINE:
                return VK_SUBPASS_CONTENTS_INLINE;
            case SECONDARY:
                return VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
            default:
                return VK_SUBPASS_CONTENTS_INLINE;
        }
    }
} // NekoEngine