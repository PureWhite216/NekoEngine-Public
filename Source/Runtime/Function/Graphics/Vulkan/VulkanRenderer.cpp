#include "VulkanRenderer.h"
#include "VulkanFactory.h"
#include "VulkanTexture.h"
#include "VulkanUtility.h"
#include "VulkanDescriptorSet.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanInitializer.h"
#include "VulkanContext.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace NekoEngine
{
    int VulkanRenderer::deletionQueueIndex = 0;
    std::vector<DeletionQueue> VulkanRenderer::deletionQueue = {};
    VkDescriptorPool VulkanRenderer::descriptorPool = {};
    int VulkanRenderer::currentDeletionQueue = 0;

    void VulkanRenderer::Init(bool loadEmbeddedShaders)
    {
        LOG("Initializing Vulkan Renderer");
//        gVulkanContext.Init();
        gVulkanContext.renderer = SharedPtr<Renderer>(this);
        rendererTitle = "VulkanRenderer";
        descriptorCapacity = 2048;

        shaderLibrary = MakeShared<ShaderLibrary>();
        rhiFactory = MakeShared<VulkanFactory>();

        LoadEngineShaders(loadEmbeddedShaders);

        std::array<VkDescriptorPoolSize, 6> poolSizes = {
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, DESCRIPTOR_MAX_SAMPLERS},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DESCRIPTOR_MAX_TEXTURES},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DESCRIPTOR_MAX_STORAGE_TEXTURES},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DESCRIPTOR_MAX_STORAGE_BUFFERS},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DESCRIPTOR_MAX_CONSTANT_BUFFERS},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DESCRIPTOR_MAX_CONSTANT_BUFFERS_DYNAMIC}
        };

        VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_SAMPLER,                100000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100000},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          100000},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          100000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   100000},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   100000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         100000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         100000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100000},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       100000}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 100000 * 11;
        pool_info.poolSizeCount = (uint32_t) 11;
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();
        poolCreateInfo.maxSets = descriptorCapacity;

        LOG("Creating Vulkan descriptor pool");
        VK_CHECK_RESULT(vkCreateDescriptorPool(GET_DEVICE(), &pool_info, nullptr, &descriptorPool),
                        "Failed to create descriptor pool!");
        // Deletion queue larger than frames in flight to delay deletion a few frames
        deletionQueue.resize(12);
        deletionQueueIndex = 0;

        LOG("Vulkan Renderer Initialized");
    }

    void VulkanRenderer::Begin()
    {
        deletionQueueIndex++;
        deletionQueueIndex = deletionQueueIndex % int(deletionQueue.size());
        deletionQueue[deletionQueueIndex].Flush();

        gVulkanContext.GetSwapChain()->Begin();
    }

    void VulkanRenderer::Present()
    {
        auto swapChain = gVulkanContext.GetSwapChain();
        swapChain->End();
        swapChain->QueueSubmit();

        auto &bufferData = swapChain->GetCurrentBufferData();
        auto semphore = bufferData.MainCommandBuffer->GetSemaphore();

        swapChain->Present(semphore);
    }

    void VulkanRenderer::ClearRenderTarget(Texture* texture, CommandBuffer* commandBuffer, Color clearColour)
    {
        VkImageSubresourceRange subresourceRange = {}; // TODO: Get from texture
        subresourceRange.baseMipLevel = 0;
        subresourceRange.layerCount = 1;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;

        if(texture->GetType() == TextureType::COLOUR)
        {
            VkImageLayout layout = (dynamic_cast<VulkanTexture2D*>(texture))->GetImageLayout();
            (dynamic_cast<VulkanTexture2D*>(texture))->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                       (VulkanCommandBuffer*) commandBuffer);

            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            VkClearColorValue clearColourValue = VkClearColorValue(
                    {{clearColour.x, clearColour.y, clearColour.z, clearColour.w}});
            vkCmdClearColorImage(((VulkanCommandBuffer*) commandBuffer)->GetHandle(),
                                 dynamic_cast<VulkanTexture2D*>(texture)->GetImage(),
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValue, 1, &subresourceRange);
            (dynamic_cast<VulkanTexture2D*>(texture))->TransitionImage(layout, (VulkanCommandBuffer*) commandBuffer);
        }
        else if(texture->GetType() == TextureType::DEPTH)
        {
            VkClearDepthStencilValue clear_depth_stencil = {1.0f, 1};

            subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            (dynamic_cast<VulkanTexture2D*>(texture))->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                       (VulkanCommandBuffer*) commandBuffer);
            vkCmdClearDepthStencilImage(((VulkanCommandBuffer*) commandBuffer)->GetHandle(),
                                        dynamic_cast<VulkanTextureDepth*>(texture)->GetImage(),
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil, 1,
                                        &subresourceRange);
        }
    }

    void VulkanRenderer::ClearSwapChainImage() const
    {
        auto m_SwapChain = gVulkanContext.GetSwapChain();
        for(int i = 0; i < m_SwapChain->GetSwapChainBufferCount(); i++)
        {
            auto cmd = VulkanUtility::BeginSingleTimeCommands();

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.layerCount = 1;
            subresourceRange.levelCount = 1;

            VkClearColorValue clearColourValue = VkClearColorValue({{0.0f, 0.0f, 0.0f, 0.0f}});

            vkCmdClearColorImage(cmd, dynamic_cast<VulkanTexture2D*>(m_SwapChain->GetImage(i))->GetImage(),
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &clearColourValue, 1, &subresourceRange);

            VulkanUtility::EndSingleTimeCommands(cmd);
        }
    }

    void
    VulkanRenderer::BindDescriptorSets(Pipeline* pipeline, CommandBuffer* commandBuffer, uint32_t dynamicOffset,
                                       DescriptorSet** descriptorSets, uint32_t descriptorCount)
    {
        uint32_t numDynamicDescriptorSets = 0;
        uint32_t numDesciptorSets = 0;

        for(uint32_t i = 0; i < descriptorCount; i++)
        {
            if(descriptorSets[i])
            {
                auto vkDesSet = dynamic_cast<VulkanDescriptorSet*>(descriptorSets[i]);
                if(vkDesSet->GetIsDynamic())
                    numDynamicDescriptorSets++;

                descriptorSetPool[numDesciptorSets] = vkDesSet->GetDescriptorSet();

                vkDesSet->GetHasUpdated(gVulkanContext.GetSwapChain()->GetCurrentBufferIndex());

                numDesciptorSets++;
            }
        }

        vkCmdBindDescriptorSets(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(),
                                dynamic_cast<VulkanPipeline*>(pipeline)->IsCompute() ? VK_PIPELINE_BIND_POINT_COMPUTE
                                                                                     : VK_PIPELINE_BIND_POINT_GRAPHICS,
                                dynamic_cast<VulkanPipeline*>(pipeline)->GetPipelineLayout(), 0, numDesciptorSets,
                                descriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);

    }


    void VulkanRenderer::OnResize(uint32_t height, uint32_t width)
    {
        if(width == 0 || height == 0) return;
        VulkanUtility::ValidateResolution(width, height);
        gVulkanContext.GetSwapChain()->OnResize(width, height, true);

    }

    void
    VulkanRenderer::Draw(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType, void* indices)
    {
        // Commit DrawCall
        vkCmdDraw(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, 0, 0);
    }

    void VulkanRenderer::DrawIndexed(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start)
    {
        // Commit DrawCall
        vkCmdDrawIndexed(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, start, 0, 0);
    }

    void VulkanRenderer::Dispatch(CommandBuffer* commandBuffer, uint32_t workGroupSizeX, uint32_t workGroupSizeY,
                                  uint32_t workGroupSizeZ)
    {
        VkCommandBuffer buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle();
        vkCmdDispatch(buffer, workGroupSizeX, workGroupSizeY, workGroupSizeZ);
    }

    uint32_t VulkanRenderer::GetGPUCount() const
    {
        return gVulkanContext.GetDevice()->GetPhysicalDevice()->GetGPUCount();
    }

    void VulkanRenderer::SaveScreenshot(const std::string &path, Texture* texture)
    {
        bool supportsBlit = true;

        // Check blit support for source and destination
        VkFormatProperties formatProps;

        // Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
        vkGetPhysicalDeviceFormatProperties(GET_GPU_HANDLE(), dynamic_cast<VulkanTexture2D*>(texture)->GetVkFormat(),
                                            &formatProps);
        if(!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
        {
            std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!"
                      << std::endl;
            supportsBlit = false;
        }

        // Check if the device supports blitting to linear images
        vkGetPhysicalDeviceFormatProperties(GET_GPU_HANDLE(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
        if(!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
        {
            std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!"
                      << std::endl;
            supportsBlit = false;
        }

        // Source for the copy is the last rendered swapchain image
        VkImage srcImage;

        if(texture)
            srcImage = dynamic_cast<VulkanTexture2D*>(texture)->GetImage();
        else
            return;

        // Create the linear tiled destination image to copy to and to read the memory from
        VkImageCreateInfo imageCreateCI = VKInitialisers::ImageCreateInfo();
        imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
        // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
        imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateCI.extent.width = texture->GetWidth();
        imageCreateCI.extent.height = texture->GetHeight();
        imageCreateCI.extent.depth = 1;
        imageCreateCI.arrayLayers = 1;
        imageCreateCI.mipLevels = 1;
        imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
        imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        // Create the image
        VkImage dstImage;
        VK_CHECK_RESULT(vkCreateImage(GET_DEVICE(), &imageCreateCI, nullptr, &dstImage), "failed to create image!");
        // Create memory to back up the image
        VkMemoryRequirements memRequirements;
        VkMemoryAllocateInfo memAllocInfo = VKInitialisers::MemoryAllocateInfo();
        VkDeviceMemory dstImageMemory;
        vkGetImageMemoryRequirements(GET_DEVICE(), dstImage, &memRequirements);
        memAllocInfo.allocationSize = memRequirements.size;
        // Memory must be host visible to copy from
        memAllocInfo.memoryTypeIndex = gVulkanContext.GetDevice()->GetPhysicalDevice()->GetMemoryTypeIndex(
                memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(GET_DEVICE(), &memAllocInfo, nullptr, &dstImageMemory),
                        "failed to allocate memory!");
        VK_CHECK_RESULT(vkBindImageMemory(GET_DEVICE(), dstImage, dstImageMemory, 0), "failed to bind image memory!");

        // Do the actual blit from the swapchain image to our host visible destination image
        VkCommandBuffer copyCmd = VulkanUtility::BeginSingleTimeCommands(); // vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        // Transition destination image to transfer destination layout
        VulkanUtility::InsertImageMemoryBarrier(
                copyCmd,
                dstImage,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        // Transition swapchain image from present to transfer source layout
        VulkanUtility::InsertImageMemoryBarrier(
                copyCmd,
                srcImage,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        // If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
        if(supportsBlit)
        {
            // Define the region to blit (we will blit the whole swapchain image)
            VkOffset3D blitSize;
            blitSize.x = texture->GetWidth();
            blitSize.y = texture->GetHeight();
            blitSize.z = 1;
            VkImageBlit imageBlitRegion{};
            imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlitRegion.srcSubresource.layerCount = 1;
            imageBlitRegion.srcOffsets[1] = blitSize;
            imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlitRegion.dstSubresource.layerCount = 1;
            imageBlitRegion.dstOffsets[1] = blitSize;

            // Issue the blit command
            vkCmdBlitImage(
                    copyCmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlitRegion,
                    VK_FILTER_NEAREST);
        }
        else
        {
            // Otherwise use image copy (requires us to manually flip components)
            VkImageCopy imageCopyRegion{};
            imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopyRegion.srcSubresource.layerCount = 1;
            imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopyRegion.dstSubresource.layerCount = 1;
            imageCopyRegion.extent.width = texture->GetWidth();
            imageCopyRegion.extent.height = texture->GetHeight();
            imageCopyRegion.extent.depth = 1;

            // Issue the copy command
            vkCmdCopyImage(
                    copyCmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageCopyRegion);
        }

        // Transition destination image to general layout, which is the required layout for mapping the image memory later on
        VulkanUtility::InsertImageMemoryBarrier(
                copyCmd,
                dstImage,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        // Transition back the swap chain image after the blit is done
        VulkanUtility::InsertImageMemoryBarrier(
                copyCmd,
                srcImage,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

        VulkanUtility::EndSingleTimeCommands(copyCmd);

        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(GET_DEVICE(), dstImage, &subResource, &subResourceLayout);

        // Map image memory so we can start copying from it
        const char* data;
        vkMapMemory(GET_DEVICE(), dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**) &data);
        data += subResourceLayout.offset;

        /*
std::ofstream file(path, std::ios::out | std::ios::binary);

// ppm header
file << "P6\n"
 << texture->GetWidth() << "\n"
 << texture->GetHeight() << "\n"
                << 255 << "\n";

        */

        // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
        bool colorSwizzle = false;
        // Check if source is BGR
        // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
        if(!supportsBlit)
        {
            std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM,
                                                VK_FORMAT_B8G8R8A8_SNORM};
            colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(),
                                      dynamic_cast<VulkanTexture2D*>(texture)->GetVkFormat()) != formatsBGR.end());
        }

        stbi_flip_vertically_on_write(1);

        uint32_t width = texture->GetWidth();
        uint32_t height = texture->GetHeight();

        int32_t resWrite = stbi_write_png(
                path.c_str(),
                width,
                height,
                4,
                data,
                (int) subResourceLayout.rowPitch);


        LOG("Screenshot saved to disk");

        // Clean up resources
        vkUnmapMemory(GET_DEVICE(), dstImageMemory);
        vkFreeMemory(GET_DEVICE(), dstImageMemory, nullptr);
        vkDestroyImage(GET_DEVICE(), dstImage, nullptr);
    }

    VkDescriptorPool VulkanRenderer::GetDescriptorPool()
    {
        return descriptorPool;
    }

    DeletionQueue &VulkanRenderer::GetCurrentDeletionQueue()
    {
        return deletionQueue[currentDeletionQueue];
    }

    DeletionQueue &VulkanRenderer::GetDeletionQueue(int index)
    {
        if(index < 0 || index >= deletionQueueIndex)
        {
            return deletionQueue[currentDeletionQueue];
        }
        return deletionQueue[index];
    }
} // NekoEngine