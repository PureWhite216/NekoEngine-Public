#include "VulkanTexture.h"
#include "VulkanUtility.h"
#include "File/ImageLoader.h"
#include "Math/Maths.h"
#include "VulkanRenderer.h"
#include "File/ImageLoader.h"
#include "VulkanDevice.h"

namespace NekoEngine
{
    static VkImageView CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageViewType viewType,
                                       VkImageAspectFlags aspectMask, uint32_t layerCount, uint32_t baseArrayLayer = 0,
                                       uint32_t baseMipLevel = 0)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
                               VK_COMPONENT_SWIZZLE_A};
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        viewInfo.subresourceRange.layerCount = layerCount;

        VkImageView imageView;
        VK_CHECK_RESULT(vkCreateImageView(gVulkanContext.GetDevice()->GetHandle(), &viewInfo, nullptr, &imageView),
                        "failed to create texture image view!");

        return imageView;
    }

    static VkSampler CreateTextureSampler(VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR,
                                          float minLod = 0.0f, float maxLod = 1.0f, bool anisotropyEnable = false,
                                          float maxAnisotropy = 1.0f,
                                          VkSamplerAddressMode modeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VkSamplerAddressMode modeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                          VkSamplerAddressMode modeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
    {
        VkSampler sampler;
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = modeU;
        samplerInfo.addressModeV = modeV;
        samplerInfo.addressModeW = modeW;
        samplerInfo.maxAnisotropy = 1;     // maxAnisotropy;
        samplerInfo.anisotropyEnable = false; // anisotropyEnable;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod = minLod;
        samplerInfo.maxLod = maxLod;

        VK_CHECK_RESULT(vkCreateSampler(gVulkanContext.GetDevice()->GetHandle(), &samplerInfo, nullptr, &sampler),
                        "failed to create texture sampler!");
    }

    static void CreateImageVMA(const VkImageCreateInfo &imageInfo, VkImage &image, VmaAllocation &allocation)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.flags = 0;
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = 0;
        allocInfo.preferredFlags = 0;
        allocInfo.memoryTypeBits = 0;
        allocInfo.pool = nullptr;
        allocInfo.pUserData = nullptr;
        vmaCreateImage(gVulkanContext.GetDevice()->GetAllocator(), &imageInfo, &allocInfo, &image, &allocation,
                       nullptr);
    }

    static void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType,
                            VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage &image, VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags,
                            VmaAllocation &allocation)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = imageType;
        imageInfo.extent = {width, height, 1};
        imageInfo.mipLevels = mipLevels;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.flags = flags;

        CreateImageVMA(imageInfo, image, allocation);
    }

    void GenerateMipmaps(CommandBuffer* commandBuffer, VkImage image, VkFormat imageFormat, uint32_t texWidth,
                         uint32_t texHeight, uint32_t mipLevels,
                         uint32_t layer = 0, uint32_t layerCount = 1)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(GET_GPU_HANDLE(), imageFormat, &formatProperties);

        if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            LOG("Texture image format does not support linear blitting!");
        }

        VkCommandBuffer vkCommandBuffer;

        if(commandBuffer)
            vkCommandBuffer = ((VulkanCommandBuffer*) commandBuffer)->GetHandle();
        else
            vkCommandBuffer = VulkanUtility::BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = layer;
        barrier.subresourceRange.layerCount = layerCount;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for(uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(vkCommandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = layer;
            blit.srcSubresource.layerCount = layerCount;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = layer;
            blit.dstSubresource.layerCount = layerCount;

            vkCmdBlitImage(vkCommandBuffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(vkCommandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            if(mipWidth > 1)
                mipWidth /= 2;
            if(mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(vkCommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        VulkanUtility::EndSingleTimeCommands(vkCommandBuffer);
    }


    VulkanTexture2D::VulkanTexture2D(TextureDesc parameters, uint32_t width, uint32_t height)
    {
        this->width = width;
        this->height = height;
        this->params = parameters;
        vkFormat = ConvertRHIFormat2VkFormat(parameters.format, parameters.srgb);
        isDeleteImage = true;
        mipMapLevels = 1;
        flags = parameters.flags;
        BuildTexture();
    }

    VulkanTexture2D::VulkanTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width,
                                     uint32_t height)
    {
        this->image = image;
        this->imageView = imageView;
        this->vkFormat = format;
        this->width = width;
        this->height = height;
        this->sampler = VK_NULL_HANDLE;
        isDeleteImage = false;
        imageMemory = VK_NULL_HANDLE;
        m_UUID = Random64::Rand(0, ULLONG_MAX);
        UpdateDescriptor();
    }

    VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, void* data, TextureDesc parameters,
                                     TextureLoadOptions loadOptions)
    {
        this->width = width;
        this->height = height;
        this->params = parameters;
        this->data = static_cast<uint8_t*>(data);
        vkFormat = ConvertRHIFormat2VkFormat(parameters.format, parameters.srgb);
        isDeleteImage = true;
        flags = parameters.flags;
        Load();
        imageView = CreateImageView(image, VulkanUtility::FormatToVK(parameters.format, parameters.srgb), mipMapLevels,
                                    VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        sampler = CreateTextureSampler(VulkanUtility::TextureFilterToVK(params.magFilter),
                                       VulkanUtility::TextureFilterToVK(params.minFilter), 0.0f,
                                       static_cast<float>(mipMapLevels), false,
                                       gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy,
                                       VulkanUtility::TextureWrapToVK(params.wrap),
                                       VulkanUtility::TextureWrapToVK(params.wrap),
                                       VulkanUtility::TextureWrapToVK(params.wrap));

        m_UUID = Random64::Rand(0, ULLONG_MAX);

        UpdateDescriptor();
    }

    VulkanTexture2D::VulkanTexture2D(const String &name, const String &filename, TextureDesc parameters,
                                     TextureLoadOptions loadOptions)
    {
        this->fileName = filename;
        this->name = name;
        this->params = parameters;
        vkFormat = ConvertRHIFormat2VkFormat(parameters.format, parameters.srgb);
        isDeleteImage = true;
        flags = parameters.flags;
        UpdateDescriptor();
    }

    VulkanTexture2D::~VulkanTexture2D()
    {
        FreeResources();
    }

    void VulkanTexture2D::FreeResources()
    {
        DeletionQueue &deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

        if(sampler)
        {
            auto tSampler = sampler;
            deletionQueue.Push([tSampler]
                               { vkDestroySampler(GET_DEVICE(), tSampler, nullptr); });
        }

        if(imageView)
        {
            auto tImageView = imageView;
            deletionQueue.Push([tImageView]
                               { vkDestroyImageView(GET_DEVICE(), tImageView, nullptr); });
        }

        for(auto &view: mipMaps)
        {
            if(view.second)
            {
                auto tImageView = view.second;
                deletionQueue.Push([tImageView]
                                   { vkDestroyImageView(GET_DEVICE(), tImageView, nullptr); });
            }
        }

        mipMaps.clear();

        if(isDeleteImage)
        {
            auto tImage = image;
            auto tAlloc = allocation;
            deletionQueue.Push([tImage, tAlloc]
                               { vmaDestroyImage(GET_ALLOCATOR(), tImage, tAlloc); });
        }

        if(stagingBuffer) delete stagingBuffer;

        imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void VulkanTexture2D::BuildTexture()
    {
        if(flags & TextureFlags::Texture_CreateMips)
        {
            mipMapLevels = static_cast<uint32_t>(std::floor((uint32_t) log2(Maths::Max(width, height)))) + 1;
        }

        CreateImage(width, height, mipMapLevels, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    image, imageMemory, 1, 0, allocation);

        imageView = CreateImageView(image, vkFormat, mipMapLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        sampler = CreateTextureSampler(ConvertRHIFilter2VkFilter(params.magFilter),
                                       ConvertRHIFilter2VkFilter(params.minFilter), 0.0f,
                                       static_cast<float>(mipMapLevels));
        imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr);

        if(flags & TextureFlags::Texture_CreateMips)
        {
            for(uint32_t i = 0; i < mipMapLevels; i++)
            {
                GetMipImageView(i);
            }
        }
        m_UUID = Random64::Rand(0, ULLONG_MAX);
    }

    VkImageView VulkanTexture2D::GetMipImageView(uint32_t mipLevel)
    {
        if(mipMaps.find(mipLevel) == mipMaps.end())
        {
            mipMaps[mipLevel] = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT,
                                                   1,
                                                   0, mipLevel);
        }
        return mipMaps.at(mipLevel);
    }

    void VulkanTexture2D::TransitionImage(VkImageLayout newLayout, VulkanCommandBuffer* commandBuffer)
    {
        if(newLayout != imageLayout)
        {
            VulkanUtility::TransitionImageLayout(image, vkFormat, imageLayout, newLayout, mipMapLevels, 1,
                                                 commandBuffer != nullptr ? commandBuffer->GetHandle() : nullptr);
            imageLayout = newLayout;
            UpdateDescriptor();
        }
    }

    void VulkanTexture2D::Resize(uint32_t width, uint32_t height)
    {
        FreeResources();

        this->width = width;
        this->height = height;
        image = VkImage();
        Handle = UUID();

        BuildTexture();
    }

    bool VulkanTexture2D::Load()
    {
        uint32_t bits;
        uint8_t* pixels;

        flags |= TextureFlags::Texture_Sampled;

        if(data == nullptr)
        {
            pixels = ImageLoader::LoadImageFromFile(fileName, &width, &height, &bits);
            if(pixels == nullptr)
            {
                LOG("Could not load image!");
                return false;
            }
            params.format = BitsToFormat(bits);
            rhiFormat = params.format;
        }
        else
        {
            if(data == nullptr) return false;
            bitsPerChannel = GetBitsFromFormat(params.format);
            bits = bitsPerChannel; //TODO: Need to Check
            pixels = data;
        }

        vkFormat = VulkanUtility::FormatToVK(params.format, params.srgb);
        auto imageSize = VkDeviceSize(width * height * bits / 8);

        if(!pixels) LOG("failed to load texture image!");

        if(flags & TextureFlags::Texture_CreateMips)
        {
            mipMapLevels = static_cast<uint32_t>(std::floor((uint32_t) log2(Maths::Max(width, height)))) + 1;
        }
        else
        {
            mipMapLevels = 1;
        }

        auto t_StagingBuffer = new VulkanBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                static_cast<uint32_t>(imageSize), pixels);

        if(data == nullptr)
        {
            delete[] pixels;
        }

        CreateImage(width, height, mipMapLevels, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                        imageMemory, 1, 0, allocation);

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipMapLevels);
        VulkanUtility::CopyBufferToImage(t_StagingBuffer->GetHandle(), image, width, height);

        delete t_StagingBuffer;

        if(flags & TextureFlags::Texture_CreateMips)
        {
            GenerateMipmaps(nullptr, image, vkFormat, width, height, mipMapLevels);
        }

        m_UUID = Random64::Rand(0, ULLONG_MAX);
        TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr);

        return true;
    }

    void VulkanTexture2D::Load(uint32_t _width, uint32_t _height, void* _data, TextureDesc _parameters, TextureLoadOptions _loadOptions)
    {
        FreeResources();

        width = _width;
        height = _height;
        params = _parameters;
        loadOptions = _loadOptions;
        data = static_cast<uint8_t*>(_data);
        rhiFormat = _parameters.format;
        vkFormat = VulkanUtility::FormatToVK(_parameters.format, _parameters.srgb);
        flags = _parameters.flags;

        Load();

        imageView = CreateImageView(image, VulkanUtility::FormatToVK(_parameters.format, _parameters.srgb),
                                    mipMapLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        sampler = CreateTextureSampler(VulkanUtility::TextureFilterToVK(_parameters.magFilter),
                                       VulkanUtility::TextureFilterToVK(_parameters.minFilter), 0.0f,
                                       static_cast<float>(mipMapLevels), _parameters.anisotropicFiltering,
                                       _parameters.anisotropicFiltering
                                       ? gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy
                                       : 1.0f, VulkanUtility::TextureWrapToVK(_parameters.wrap),
                                       VulkanUtility::TextureWrapToVK(_parameters.wrap),
                                       VulkanUtility::TextureWrapToVK(_parameters.wrap));

        m_UUID = Random64::Rand(0, std::numeric_limits<uint64_t>::max());

        UpdateDescriptor();
    }

    void VulkanTexture2D::SetData(const void* pixels)
    {
        auto imageSize = VkDeviceSize(width * height * bitsPerChannel / 2); // / 8);

        if(!pixels)
        {
            LOG("failed to load texture image!");
        }

        mipMapLevels = 1;

        if(!stagingBuffer)
            stagingBuffer = new VulkanBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                             static_cast<uint32_t>(imageSize), pixels);
        else
            stagingBuffer->SetData(pixels, static_cast<uint32_t>(imageSize));

        TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VulkanUtility::CopyBufferToImage(stagingBuffer->GetBuffer(), image, static_cast<uint32_t>(width),
                                         static_cast<uint32_t>(height));

        TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanTexture2D::UpdateDescriptor()
    {
        descriptor.sampler = sampler;
        descriptor.imageView = imageView;
        descriptor.imageLayout = imageLayout;
    }

    VulkanTextureDepth::VulkanTextureDepth(uint32_t _width, uint32_t _height)
    {
        width = _width;
        height = _height;
        params = TextureDesc();
        mipMapLevels = 1;
        flags |= TextureFlags::Texture_DepthStencil;

        auto depthFormat = VulkanUtility::FindDepthFormat();
        vkFormat = depthFormat;
        rhiFormat = VulkanUtility::VKToFormat(depthFormat);

        BuildTexture();
    }

    VulkanTextureDepth::~VulkanTextureDepth()
    {
        FreeResources();
    }

    void VulkanTextureDepth::Resize(uint32_t _width, uint32_t _height)
    {
        width  = _width;
        height = _height;
        Handle   = UUID();

        DeletionQueue& deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

        auto tSampler   = sampler;
        auto tImageView = imageView;

        deletionQueue.Push([tSampler, tImageView]
                                   {
                                       vkDestroySampler(GET_DEVICE(), tSampler, nullptr);
                                       vkDestroyImageView(GET_DEVICE(), tImageView, nullptr); });

        auto textureImage = image;
        auto alloc        = allocation;

        deletionQueue.Push([textureImage, alloc]
                                   { vmaDestroyImage(gVulkanContext.GetDevice()->GetAllocator(), textureImage, alloc); });

        image = VkImage();

        Init();
    }

    void VulkanTextureDepth::Init()
    {
        VkFormat depthFormat = VulkanUtility::FindDepthFormat();
        vkFormat           = depthFormat;
        rhiFormat             = VulkanUtility::VKToFormat(depthFormat);

        CreateImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, 1, 0, allocation);

        imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, 1.0f, false, gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

        flags |= TextureFlags::Texture_DepthStencil;

        UpdateDescriptor();

        m_UUID = Random64::Rand(0, std::numeric_limits<uint64_t>::max());
    }

    void VulkanTextureDepth::BuildTexture()
    {
        CreateImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, 1, 0,
                    allocation);
        imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, 1.0f, false,
                                       gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy,
                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        UpdateDescriptor();
        m_UUID = Random64::Rand(0, ULLONG_MAX);

    }

    VulkanTextureDepthArray::VulkanTextureDepthArray(uint32_t width, uint32_t height, uint32_t count)
    {
        this->width = width;
        this->height = height;
        this->count = count;
        params = TextureDesc();
        mipMapLevels = 1;

        flags |= TextureFlags::Texture_DepthStencil;
        vkFormat = VK_FORMAT_D32_SFLOAT;
        rhiFormat = VulkanUtility::VKToFormat(vkFormat);

        BuildTexture();
    }

    VulkanTextureDepthArray::~VulkanTextureDepthArray()
    {

    }

    void VulkanTextureDepthArray::Resize(uint32_t _width, uint32_t _height, uint32_t _count)
    {
        width  = _width;
        height = _height;
        count = _count;
        Handle   = UUID();

        DeletionQueue& deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

        auto tSampler   = sampler;
        auto tImageView = imageView;
        auto tImageViews = individualImageViews;

        deletionQueue.Push([tSampler, tImageView, tImageViews]
                           {
                               vkDestroySampler(GET_DEVICE(), tSampler, nullptr);
                               vkDestroyImageView(GET_DEVICE(), tImageView, nullptr);
                               for(uint32_t i = 0; i < (uint32_t)tImageViews.size(); i++)
                               {
                                   vkDestroyImageView(GET_DEVICE(), tImageViews[i], nullptr);
                               }
                           });

        auto textureImage = image;
        auto alloc        = allocation;

        deletionQueue.Push([textureImage, alloc]
                           { vmaDestroyImage(gVulkanContext.GetDevice()->GetAllocator(), textureImage, alloc); });

        image = VkImage();

        Init();
    }

    void VulkanTextureDepthArray::Init()
    {
        flags |= TextureFlags::Texture_DepthStencil;
        vkFormat           = VK_FORMAT_D32_SFLOAT;
        rhiFormat             = VulkanUtility::VKToFormat(vkFormat);

        CreateImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, count, 0, allocation);

        imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, count);

        for(uint32_t i = 0; i < count; i++)
        {
            VkImageView imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, 1, i);
            individualImageViews.push_back(imageView);
        }

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, count);

        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        sampler = CreateTextureSampler();

        m_UUID = Random64::Rand(0, std::numeric_limits<uint64_t>::max());

        UpdateDescriptor();
    }

    void VulkanTextureDepthArray::BuildTexture() ///////
    {
        CreateImage(width, height, 1, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, count, 0,
                    allocation);
        imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                                    VK_IMAGE_ASPECT_DEPTH_BIT, count);

        for(uint32_t i = 0; i < count; i++)
        {
            VkImageView imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                                                    VK_IMAGE_ASPECT_DEPTH_BIT, 1, i);
            individualImageViews.push_back(imageView);
        }

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, count);
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        sampler = CreateTextureSampler();
        m_UUID = Random64::Rand(0, ULLONG_MAX);
        UpdateDescriptor();
    }

    VulkanTextureCube::VulkanTextureCube(uint32_t size, void* data, bool hdr)
    {
        imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        params = TextureDesc();
        rhiFormat = hdr ? RHIFormat::R32G32B32A32_Float : RHIFormat::R8G8B8A8_Unorm;
        params.format = rhiFormat;
        vkFormat = VulkanUtility::FormatToVK(rhiFormat, params.srgb);

        width = size;
        height = size;
        data = (uint8_t*) data;
        mipsNums = static_cast<uint32_t>(std::floor(std::log2(Maths::Max(width, height)))) + 1;
        layersNum = 6;
        isDeleteImage = true;

        flags |= TextureFlags::Texture_Sampled;

        bitsPerChannel = hdr ? 32 : 8;
        channelCount = 4;

        BuildTexture();
    }

    VulkanTextureCube::VulkanTextureCube(const std::string &filepath)
    {
        files[0] = filepath;
        rhiFormat = params.format;
    }

    VulkanTextureCube::VulkanTextureCube(const std::string* _files)
    {
        for(uint32_t i = 0; i < 6; i++)
            files[i] = _files[i];

        Load(1);

        UpdateDescriptor();
    }

    VulkanTextureCube::VulkanTextureCube(const std::string* _files, uint32_t mips, TextureDesc _params,
                                         TextureLoadOptions loadOptions)
    {
        params = _params;
        mipsNums = mips;
        for(uint32_t i = 0; i < mips; i++)
            files[i] = _files[i];

        Load(mips);
        rhiFormat = params.format;

        UpdateDescriptor();
    }

    void VulkanTextureCube::BuildTexture()
    {
        uint32_t dataSize = width * height * GetBytesPerPixel() * layersNum;
        uint8_t* allData = new uint8_t[dataSize];
        uint32_t pointeroffset = 0;

        uint32_t faceOrder[6] = {3, 1, 0, 4, 2, 5};

        CreateImage(width, height, mipsNums, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory,
                    layersNum, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, allocation);
        sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, static_cast<float>(mipsNums), false,
                                       gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT);
        imageView = CreateImageView(image, vkFormat, mipsNums, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT,
                                    layersNum);

        if(data)
        {
            memcpy(allData, data, dataSize);

            VulkanBuffer* stagingBuffer = new VulkanBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                           static_cast<uint32_t>(dataSize), allData);

            if(data == nullptr)
            {
                delete[] allData;
                allData = nullptr;
            }

            VkCommandBuffer commandBuffer = VulkanUtility::BeginSingleTimeCommands();

            //// Setup buffer copy regions for each face including all of it's miplevels
            std::vector<VkBufferImageCopy> bufferCopyRegions;
            uint32_t offset = 0;

            for(uint32_t face = 0; face < layersNum; face++)
            {
                for(uint32_t level = 0; level < mipsNums; level++)
                {
                    VkBufferImageCopy bufferCopyRegion = {};
                    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    bufferCopyRegion.imageSubresource.mipLevel = level;
                    bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                    bufferCopyRegion.imageSubresource.layerCount = 1;
                    bufferCopyRegion.imageExtent.width = width;
                    bufferCopyRegion.imageExtent.height = height;
                    bufferCopyRegion.imageExtent.depth = 1;
                    bufferCopyRegion.bufferOffset = offset;

                    bufferCopyRegions.push_back(bufferCopyRegion);

                    // Increase offset into staging buffer for next level / face
                    offset += width * height * GetBytesPerPixel();
                }
            }

            // Image barrier for optimal image (target)
            // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = mipsNums;
            subresourceRange.layerCount = layersNum;

            VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipsNums, layersNum,
                                                 commandBuffer);

            // Copy the cube map faces from the staging buffer to the optimal tiled image
            vkCmdCopyBufferToImage(
                    commandBuffer,
                    stagingBuffer->GetBuffer(),
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    static_cast<uint32_t>(bufferCopyRegions.size()),
                    bufferCopyRegions.data());

            // Change texture image layout to shader read after all faces have been copied
            VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipsNums, layersNum,
                                                 commandBuffer);

            imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VulkanUtility::EndSingleTimeCommands(commandBuffer);

            delete stagingBuffer;
        }
        delete[] allData;

        for(uint32_t i = 0; i < layersNum; i++)
        {
            VkImageView imageView = CreateImageView(image, vkFormat, 1, VK_IMAGE_VIEW_TYPE_2D,
                                                    VK_IMAGE_ASPECT_COLOR_BIT, 1, i, 0);
            individualImageViews.push_back(imageView);
        }

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, mipsNums, layersNum, nullptr);

        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        m_UUID = Random64::Rand(0, ULLONG_MAX);

        UpdateDescriptor();
    }

    VulkanTextureCube::~VulkanTextureCube()
    {
        FreeResources();
    }

    void VulkanTextureCube::FreeResources()
    {
        DeletionQueue &deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

        if(sampler)
        {
            auto tSampler = sampler;
            deletionQueue.Push([tSampler]
                               { vkDestroySampler(GET_DEVICE(), tSampler, nullptr); });
        }

        if(imageView)
        {
            auto tImageView = imageView;
            deletionQueue.Push([tImageView]
                               { vkDestroyImageView(GET_DEVICE(), tImageView, nullptr); });
        }

        if(!individualImageViews.empty())
        {
            auto tImageViews = individualImageViews;
            deletionQueue.Push([tImageViews]
                               {
                                   for(auto &view: tImageViews)
                                       vkDestroyImageView(GET_DEVICE(), view, nullptr);
                               });
        }


        if(isDeleteImage)
        {
            auto tImage = image;
            auto tAlloc = allocation;
            deletionQueue.Push([tImage, tAlloc]
                               { vmaDestroyImage(GET_ALLOCATOR(), tImage, tAlloc); });
        }
    }

    void VulkanTextureCube::TransitionImage(VkImageLayout newLayout, VulkanCommandBuffer* commandBuffer)
    {
        if(newLayout != imageLayout)
            VulkanUtility::TransitionImageLayout(image, vkFormat, imageLayout, newLayout, mipsNums, 6,
                                                 commandBuffer ? commandBuffer->GetHandle() : nullptr);
        imageLayout = newLayout;
        UpdateDescriptor();
    }

    void VulkanTextureCube::UpdateDescriptor()
    {
        descriptor.sampler = sampler;
        descriptor.imageView = imageView;
        descriptor.imageLayout = imageLayout;
    }

    void VulkanTextureCube::Load(uint32_t mips)
    {
        uint32_t srcWidth, srcHeight, bits = 0;
        uint8_t*** cubeTextureData = new uint8_t** [mips];
        for(uint32_t i = 0; i < mips; i++)
            cubeTextureData[i] = new uint8_t* [6];

        uint32_t* faceWidths = new uint32_t[mips];
        uint32_t* faceHeights = new uint32_t[mips];
        uint32_t size = 0;
        bool isHDR = false;

        flags |= TextureFlags::Texture_Sampled;

        for(uint32_t m = 0; m < mips; m++)
        {
            uint8_t* data = ImageLoader::LoadImageFromFile(files[m], &srcWidth, &srcHeight, &bits, &isHDR,
                                                           !loadOptions.flipY);
            params.format = BitsToFormat(bits);

            uint32_t face = 0;
            uint32_t faceWidth = srcWidth / 3;
            uint32_t faceHeight = srcHeight / 4;

            if(m == 0)
            {
                width = faceWidth;
                height = faceHeight;
                bitsPerChannel = bits;
                channelCount = BitsToChannelCount(bits);
                vkFormat = VulkanUtility::FormatToVK(params.format, params.srgb);
            }

            uint32_t stride = GetBytesPerChannel();

            faceWidths[m] = faceWidth;
            faceHeights[m] = faceHeight;
            for(uint32_t cy = 0; cy < 4; cy++)
            {
                for(uint32_t cx = 0; cx < 3; cx++)
                {
                    if(cy == 0 || cy == 2 || cy == 3)
                    {
                        if(cx != 1)
                            continue;
                    }

                    cubeTextureData[m][face] = new uint8_t[faceWidth * faceHeight * stride];

                    size += stride * srcHeight * srcWidth;

                    for(uint32_t y = 0; y < faceHeight; y++)
                    {
                        uint32_t offset = y;
                        if(face == 5)
                            offset = faceHeight - (y + 1);
                        uint32_t yp = cy * faceHeight + offset;
                        for(uint32_t x = 0; x < faceWidth; x++)
                        {
                            offset = x;
                            if(face == 5)
                                offset = faceWidth - (x + 1);
                            uint32_t xp = cx * faceWidth + offset;
                            cubeTextureData[m][face][(x + y * faceWidth) * stride + 0] = data[
                                    (xp + yp * srcWidth) * stride + 0];
                            cubeTextureData[m][face][(x + y * faceWidth) * stride + 1] = data[
                                    (xp + yp * srcWidth) * stride + 1];
                            cubeTextureData[m][face][(x + y * faceWidth) * stride + 2] = data[
                                    (xp + yp * srcWidth) * stride + 2];
                            if(stride >= 4)
                                cubeTextureData[m][face][(x + y * faceWidth) * stride + 3] = data[
                                        (xp + yp * srcWidth) * stride + 3];
                        }
                    }
                    face++;
                }
            }
            delete[] data;
        }

        uint8_t* allData = new uint8_t[size];
        uint32_t pointeroffset = 0;

        uint32_t faceOrder[6] = {3, 1, 0, 4, 2, 5};

        for(uint32_t face = 0; face < 6; face++)
        {
            for(uint32_t mip = 0; mip < mipsNums; mip++)
            {
                uint32_t currentSize = faceWidths[mip] * faceHeights[mip] * GetBytesPerChannel();
                memcpy(allData + pointeroffset, cubeTextureData[mip][faceOrder[face]], currentSize);
                pointeroffset += currentSize;
            }
        }

        auto* stagingBuffer = new VulkanBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                               static_cast<uint32_t>(size), allData);

        if(data == nullptr)
        {
            delete[] allData;
            allData = nullptr;
        }

        CreateImage(faceWidths[0], faceHeights[0], mipsNums, vkFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                    allocation);


        VkCommandBuffer commandBuffer = VulkanUtility::BeginSingleTimeCommands();

        //// Setup buffer copy regions for each face including all of it's miplevels
        std::vector<VkBufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        for(uint32_t face = 0; face < 6; face++)
        {
            for(uint32_t level = 0; level < mipsNums; level++)
            {
                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = faceWidths[level];
                bufferCopyRegion.imageExtent.height = faceHeights[level];
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);

                // Increase offset into staging buffer for next level / face
                offset += faceWidths[level] * faceWidths[level] * GetBytesPerChannel();
            }
        }

        // Image barrier for optimal image (target)
        // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipsNums;
        subresourceRange.layerCount = 6;

        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipsNums, 6, commandBuffer);

        // Copy the cube map faces from the staging buffer to the optimal tiled image
        vkCmdCopyBufferToImage(
                commandBuffer,
                stagingBuffer->GetBuffer(),
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(bufferCopyRegions.size()),
                bufferCopyRegions.data());

        // Change texture image layout to shader read after all faces have been copied
        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipsNums, 6, commandBuffer);

        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VulkanUtility::EndSingleTimeCommands(commandBuffer);

        sampler = CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, static_cast<float>(mipsNums), false,
                                       gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT);
        imageView = CreateImageView(image, vkFormat, mipsNums, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, 6);

        m_UUID = Random64::Rand(0, ULLONG_MAX);

        delete stagingBuffer;

        for(uint32_t m = 0; m < mips; m++)
        {
            for(uint32_t f = 0; f < 6; f++)
            {
                delete[] cubeTextureData[m][f];
            }
            delete[] cubeTextureData[m];
        }
        delete[] cubeTextureData;
        delete[] faceHeights;
        delete[] faceWidths;
        delete[] allData;

        for(uint32_t i = 0; i < 6; i++)
        {
            VkImageView tImageView = CreateImageView(image, VulkanUtility::FormatToVK(params.format, params.srgb), 1,
                                                     VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1, i);

            individualImageViews.push_back(tImageView);
        }
    }

    void VulkanTextureCube::GenerateMipMaps(CommandBuffer* commandBuffer)
    {
        VulkanUtility::TransitionImageLayout(image, vkFormat, imageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             mipsNums, 6, ((VulkanCommandBuffer*) commandBuffer)->GetHandle());

        for(int i = 0; i < 6; i++)
            GenerateMipmaps(commandBuffer, image, vkFormat, width, height, mipsNums, i, 1);

        // Generate mips sets layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        VulkanUtility::TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageLayout,
                                             mipsNums, 6, ((VulkanCommandBuffer*) commandBuffer)->GetHandle());
    }


    VkFormat ConvertRHIFormat2VkFormat(RHIFormat format, bool isSRGB)
    {
        {
            if(isSRGB)
            {
                switch(format)
                {
                    case RHIFormat::R8_Unorm:
                        return VK_FORMAT_R8_SRGB;
                    case RHIFormat::R8G8_Unorm:
                        return VK_FORMAT_R8G8_SRGB;
                    case RHIFormat::R8G8B8_Unorm:
                        return VK_FORMAT_R8G8B8_SRGB;
                    case RHIFormat::R8G8B8A8_Unorm:
                        return VK_FORMAT_R8G8B8A8_SRGB;
                    case RHIFormat::R16G16B16_Float:
                        return VK_FORMAT_R16G16B16_SFLOAT;
                    case RHIFormat::R16G16B16A16_Float:
                        return VK_FORMAT_R16G16B16A16_SFLOAT;
                    case RHIFormat::R32G32B32_Float:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    case RHIFormat::R32G32B32A32_Float:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    default:
                        LOG("Unsupported image bit-depth!");
                        return VK_FORMAT_R8G8B8A8_SRGB;
                }
            }
            else
            {
                switch(format)
                {
                    case RHIFormat::R8_Unorm:
                        return VK_FORMAT_R8_UNORM;
                    case RHIFormat::R8G8_Unorm:
                        return VK_FORMAT_R8G8_UNORM;
                    case RHIFormat::R8G8B8_Unorm:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                    case RHIFormat::R8G8B8A8_Unorm:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                    case RHIFormat::R11G11B10_Float:
                        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                    case RHIFormat::R10G10B10A2_Unorm:
                        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
                    case RHIFormat::R16_Float:
                        return VK_FORMAT_R16_SFLOAT;
                    case RHIFormat::R16G16_Float:
                        return VK_FORMAT_R16G16_SFLOAT;
                    case RHIFormat::R16G16B16_Float:
                        return VK_FORMAT_R16G16B16_SFLOAT;
                    case RHIFormat::R16G16B16A16_Float:
                        return VK_FORMAT_R16G16B16A16_SFLOAT;
                    case RHIFormat::R32_Float:
                        return VK_FORMAT_R32_SFLOAT;
                    case RHIFormat::R32G32_Float:
                        return VK_FORMAT_R32G32_SFLOAT;
                    case RHIFormat::R32G32B32_Float:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    case RHIFormat::R32G32B32A32_Float:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    case RHIFormat::D16_Unorm:
                        return VK_FORMAT_D16_UNORM;
                    case RHIFormat::D32_Float:
                        return VK_FORMAT_D32_SFLOAT;
                    case RHIFormat::D24_Unorm_S8_UInt:
                        return VK_FORMAT_D24_UNORM_S8_UINT;
                    case RHIFormat::D32_Float_S8_UInt:
                        return VK_FORMAT_D32_SFLOAT_S8_UINT;
                    default:
                        LOG("Unsupported image bit-depth!");
                        return VK_FORMAT_R8G8B8A8_UNORM;
                }
            }
        }
    }

    VkFilter ConvertRHIFilter2VkFilter(const TextureFilter filter)
    {
        switch(filter)
        {
            case TextureFilter::LINEAR:
                return VK_FILTER_LINEAR;
            case TextureFilter::NEAREST:
                return VK_FILTER_NEAREST;
            case TextureFilter::NONE:
                return VK_FILTER_LINEAR;
            default:
                LOG("Unsupported filter!");
                return VK_FILTER_LINEAR;
        }
    }

} // NekoEngine