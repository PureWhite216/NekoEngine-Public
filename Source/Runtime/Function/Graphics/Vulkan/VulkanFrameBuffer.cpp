#include "VulkanFramebuffer.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderer.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    VulkanFramebuffer::VulkanFramebuffer(const FramebufferDesc& desc)
    {
        width = desc.width;
        height = desc.height;
        attachmentCount = desc.attachmentCount;
        ArrayList<VkImageView> attachments;

        for (uint32_t i = 0; i < attachmentCount; i++)
        {
            //TODO support cube depth
            switch(desc.attachmentTypes[i])
            {
                case TextureType::COLOUR:
                    attachments.push_back(desc.mipIndex >= 0 ? dynamic_cast<VulkanTexture2D*>(desc.attachments[i])->GetMipImageView(desc.mipIndex) : dynamic_cast<VulkanTexture2D*>(desc.attachments[i])->GetImageView());
                    break;
//                case TextureType::DEPTH:
//                    attachments.push_back(static_cast<VulkanTextureDepth*>(desc.attachments[i])->GetImageView());
//                    break;
//                case TextureType::DEPTHARRAY:
//                    attachments.push_back(static_cast<VulkanTextureDepthArray*>(desc.attachments[i])->GetImageView(desc.layer));
//                    break;
//                case TextureType::OTHER:
//                    attachments.push_back(static_cast<VulkanTexture2D*>(desc.attachments[i])->GetImageView());
//                    break;
//                case TextureType::CUBE:
//                    attachments.push_back(static_cast<VulkanTextureCube*>(desc.attachments[i])->GetImageView(desc.layer));
//                    break;
            }

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = dynamic_cast<VulkanRenderPass*>(desc.renderPass)->GetHandle();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;
            framebufferInfo.flags = 0;

            VK_CHECK_RESULT(vkCreateFramebuffer(GET_DEVICE(), &framebufferInfo, nullptr, &handle), "failed to create framebuffer!");
        }


    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        DeletionQueue& deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();
        deletionQueue.Push([=]()
        {
            vkDestroyFramebuffer(GET_DEVICE(), handle, nullptr);
        });
    }


} // NekoEngine