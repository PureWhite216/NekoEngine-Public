#pragma once
#include "RHI/DescriptorSet.h"
#include "Buffer.h"
#include "Vk.h"

#define MAX_BUFFER_INFOS 32
#define MAX_IMAGE_INFOS 32
#define MAX_WRITE_DESCRIPTORS 32

namespace NekoEngine
{
    struct UniformBufferInfo
    {
        std::vector<BufferMemberInfo> m_Members;
        Buffer LocalStorage;

        // Per frame in flight
        bool HasUpdated[10];
    };

    class VulkanDescriptorSet : public DescriptorSet
    {
    private:
        uint32_t m_DynamicOffset = 0;
        Shader* m_Shader         = nullptr;
        bool m_Dynamic           = false;
        std::array<VkDescriptorBufferInfo, MAX_BUFFER_INFOS> m_BufferInfoPool;
        std::array<VkDescriptorImageInfo, MAX_IMAGE_INFOS> m_ImageInfoPool;
        std::array<VkWriteDescriptorSet, MAX_WRITE_DESCRIPTORS> m_WriteDescriptorSetPool;

        uint32_t m_FramesInFlight = 0;


        std::map<uint32_t, VkDescriptorSet> m_DescriptorSet;
        DescriptorSetInfo m_Descriptors;
        std::map<uint32_t, std::map<std::string, SharedPtr<UniformBuffer>>> m_UniformBuffers;

        std::map<std::string, UniformBufferInfo> m_UniformBuffersData;
        bool m_DescriptorDirty[3];
        bool m_DescriptorUpdated[3];

    public:
        VulkanDescriptorSet(const DescriptorDesc& descriptorDesc);
        ~VulkanDescriptorSet();

        VkDescriptorSet GetDescriptorSet();
        void Update(CommandBuffer* cmdBuffer) override;
        void SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex, TextureType textureType) override;
        void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType) override;
        void SetBuffer(const std::string& name, UniformBuffer* buffer) override;
        void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data) override;
        void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) override;
        void SetUniformBufferData(const std::string& bufferName, void* data) override;
        void TransitionImages(CommandBuffer* commandBuffer) override;

        UniformBuffer* GetUnifromBuffer(const std::string& name) override;
        bool GetIsDynamic() const { return m_Dynamic; }

        void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
        uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
        bool GetHasUpdated(uint32_t frame) { return m_DescriptorUpdated[frame]; }
        void SetUniformDynamic(const std::string& bufferName, uint32_t size) override;
    protected:
        void UpdateInternal(std::vector<Descriptor>* imageInfos);
    };

} // NekoEngine

