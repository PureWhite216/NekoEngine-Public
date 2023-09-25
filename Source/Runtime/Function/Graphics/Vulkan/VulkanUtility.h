#pragma once

#include "Vk.h"
#include "Core.h"
#include "RHI/Definitions.h"

namespace NekoEngine
{
    namespace VulkanUtility
    {
        void Init();
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VkDeviceMemory &bufferMemory, VmaAllocator allocator = nullptr,
                     VmaAllocation allocation = nullptr);

        VkCommandBuffer BeginSingleTimeCommands();

        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        bool HasStencilComponent(VkFormat format);

        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                                   uint32_t mipLevels = 1, uint32_t layerCount = 1,
                                   VkCommandBuffer commandBuffer = nullptr);

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                     VkFormatFeatureFlags features);

        VkFormat FindDepthFormat();

        void ValidateResolution(uint32_t &width, uint32_t &height);

        bool IsPresentModeSupported(const std::vector<VkPresentModeKHR> &supportedModes, VkPresentModeKHR presentMode);

        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &supportedModes, bool vsync);

        void WaitIdle();

        std::string ErrorString(VkResult errorCode);

        VkVertexInputAttributeDescription VertexInputDescriptionToVK(VertexInputDescription description);

        VkCullModeFlags CullModeToVK(CullMode mode);

        VkDescriptorType DescriptorTypeToVK(DescriptorType type);

        VkFormat FormatToVK(const RHIFormat format, bool srgb = false);

        RHIFormat VKToFormat(VkFormat format);

        VkSamplerAddressMode TextureWrapToVK(const TextureWrap format);

        VkFilter TextureFilterToVK(const TextureFilter filter);

        VkShaderStageFlagBits ShaderTypeToVK(const ShaderType &shaderName);

        VkPolygonMode PolygonModeToVk(PolygonMode mode);

        VkPrimitiveTopology DrawTypeToVk(DrawType type);

        void InsertImageMemoryBarrier(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkImageSubresourceRange subresourceRange);

        void SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const std::string& name, const void* handle);

    }
} // NekoEngine

