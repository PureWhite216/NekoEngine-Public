#include "VulkanDescriptorSet.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanUniformBuffer.h"
#include "VulkanTexture.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    uint32_t g_DescriptorSetCount = 0;

    VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorDesc &descriptorDesc)
    {
        m_FramesInFlight = uint32_t(gVulkanContext.GetSwapChain()->GetSwapChainBufferCount());

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = VulkanRenderer::GetDescriptorPool();
        descriptorSetAllocateInfo.pSetLayouts = dynamic_cast<VulkanShader*>(descriptorDesc.shader)->GetDescriptorLayout(
                descriptorDesc.layoutIndex);
        descriptorSetAllocateInfo.descriptorSetCount = descriptorDesc.count;
        descriptorSetAllocateInfo.pNext = nullptr;

        m_Shader = descriptorDesc.shader;
        m_Descriptors = m_Shader->GetDescriptorInfo(descriptorDesc.layoutIndex);

        for(auto &descriptor: m_Descriptors.descriptors)
        {
            if(descriptor.type == DescriptorType::UNIFORM_BUFFER)
            {
                for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
                {
                    // Uniform Buffer per frame in flight
                    auto buffer = SharedPtr<UniformBuffer>(new VulkanUniformBuffer());
                    buffer->Init(descriptor.size, nullptr);
                    m_UniformBuffers[frame][descriptor.name] = buffer;
                }

                Buffer localStorage;
                localStorage.Allocate(descriptor.size);
                localStorage.InitialiseEmpty();

                UniformBufferInfo info;
                info.LocalStorage = localStorage;
                info.HasUpdated[0] = false;
                info.HasUpdated[1] = false;
                info.HasUpdated[2] = false;
                info.m_Members = descriptor.m_Members;
                m_UniformBuffersData[descriptor.name] = info;
            }
        }

        for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
        {
            m_DescriptorDirty[frame] = true;
            m_DescriptorUpdated[frame] = false;
            m_DescriptorSet[frame] = nullptr;
            g_DescriptorSetCount++;
            VK_CHECK_RESULT(vkAllocateDescriptorSets(GET_DEVICE(), &descriptorSetAllocateInfo, &m_DescriptorSet[frame]),
                            "Failed to allocate descriptor set!");
        }
    }

    VulkanDescriptorSet::~VulkanDescriptorSet()
    {
        for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
        {
            if(!m_DescriptorSet[frame])
                continue;

            auto descriptorSet = m_DescriptorSet[frame];
            auto pool = VulkanRenderer::GetDescriptorPool();
            auto device = GET_DEVICE();
            std::map<std::string, SharedPtr<UniformBuffer>> buffers = m_UniformBuffers[frame];

            DeletionQueue &deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();
            deletionQueue.Push([descriptorSet, pool, device]
                                       { vkFreeDescriptorSets(device, pool, 1, &descriptorSet); });
        }

        for(auto it = m_UniformBuffersData.begin(); it != m_UniformBuffersData.end(); it++)
        {
            it->second.LocalStorage.Release();
        }

        g_DescriptorSetCount -= 3;
    }
    
    
    void TransitionImageToCorrectLayout(Texture* texture)
    {
        if(!texture)
            return;

        auto commandBuffer = gVulkanContext.GetSwapChain()->GetCurrentCommandBuffer();
        if(texture->GetType() == TextureType::COLOUR)
        {
            if((dynamic_cast<VulkanTexture2D*>(texture))->GetImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dynamic_cast<VulkanTexture2D*>(texture)->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                          (VulkanCommandBuffer*) commandBuffer);
            }
        }
        if(texture->GetType() == TextureType::CUBE)
        {
            if(((VulkanTextureCube*) texture)->GetImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                ((VulkanTextureCube*)texture)->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                            (VulkanCommandBuffer*) commandBuffer);
            }
        }
        else if(texture->GetType() == TextureType::DEPTH)
        {
            dynamic_cast<VulkanTextureDepth*>(texture)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                                         (VulkanCommandBuffer*) commandBuffer);
        }
        else if(texture->GetType() == TextureType::DEPTHARRAY)
        {
            dynamic_cast<VulkanTextureDepthArray*>(texture)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                                              (VulkanCommandBuffer*) commandBuffer);
        }
    }

    void VulkanDescriptorSet::Update(CommandBuffer* cmdBuffer)
    {
        m_Dynamic = false;
        int descriptorWritesCount = 0;
        uint32_t currentFrame = gVulkanContext.GetSwapChain()->GetCurrentBufferIndex();

        for(auto &bufferInfo: m_UniformBuffersData)
        {
            if(bufferInfo.second.HasUpdated[currentFrame])
            {
                m_UniformBuffers[currentFrame][bufferInfo.first]->SetData(bufferInfo.second.LocalStorage.Data);
                bufferInfo.second.HasUpdated[currentFrame] = false;
            }
        }

        if(m_DescriptorDirty[currentFrame] || !m_DescriptorUpdated[currentFrame])
        {
            m_DescriptorDirty[currentFrame] = false;
            int imageIndex = 0;
            int index = 0;

            for(auto &imageInfo: m_Descriptors.descriptors)
            {
                if(imageInfo.type == DescriptorType::IMAGE_SAMPLER && (imageInfo.texture || imageInfo.textures))
                {
                    if(imageInfo.textureCount == 1)
                    {
                        if(imageInfo.texture)
                        {
                            TransitionImageToCorrectLayout(imageInfo.texture);

                            VkDescriptorImageInfo &des = *static_cast<VkDescriptorImageInfo*>(imageInfo.texture->GetDescriptorInfo());
                            m_ImageInfoPool[imageIndex].imageLayout = des.imageLayout;
                            m_ImageInfoPool[imageIndex].imageView = des.imageView;
                            m_ImageInfoPool[imageIndex].sampler = des.sampler;
                        }
                    }
                    else
                    {
                        if(imageInfo.textures)
                        {
                            for(uint32_t i = 0; i < imageInfo.textureCount; i++)
                            {
                                TransitionImageToCorrectLayout(imageInfo.textures[i]);

                                VkDescriptorImageInfo &des = *static_cast<VkDescriptorImageInfo*>(imageInfo.textures[i]->GetDescriptorInfo());
                                m_ImageInfoPool[i + imageIndex].imageLayout = des.imageLayout;
                                m_ImageInfoPool[i + imageIndex].imageView = des.imageView;
                                m_ImageInfoPool[i + imageIndex].sampler = des.sampler;
                            }
                        }
                    }

                    VkWriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = m_DescriptorSet[currentFrame];
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptorSet.dstBinding = imageInfo.binding;
                    writeDescriptorSet.pImageInfo = &m_ImageInfoPool[imageIndex];
                    writeDescriptorSet.descriptorCount = imageInfo.textureCount;

                    m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                    imageIndex++;
                    descriptorWritesCount++;
                }
                else if(imageInfo.type == DescriptorType::IMAGE_STORAGE && imageInfo.texture)
                {
                    if(imageInfo.texture)
                    {
                        dynamic_cast<VulkanTexture2D*>(imageInfo.texture)->TransitionImage(VK_IMAGE_LAYOUT_GENERAL);

                        VkDescriptorImageInfo &des = *static_cast<VkDescriptorImageInfo*>(imageInfo.texture->GetDescriptorInfo());
                        m_ImageInfoPool[imageIndex].imageLayout = des.imageLayout;
                        m_ImageInfoPool[imageIndex].imageView =
                                imageInfo.mipLevel > 0 ? dynamic_cast<VulkanTexture2D*>(imageInfo.texture)->GetMipImageView(
                                        imageInfo.mipLevel) : des.imageView;
                        m_ImageInfoPool[imageIndex].sampler = des.sampler;
                    }

                    VkWriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = m_DescriptorSet[currentFrame];
                    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    writeDescriptorSet.dstBinding = imageInfo.binding;
                    writeDescriptorSet.pImageInfo = &m_ImageInfoPool[imageIndex];
                    writeDescriptorSet.descriptorCount = imageInfo.textureCount;

                    m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                    imageIndex++;
                    descriptorWritesCount++;
                }

                else if(imageInfo.type == DescriptorType::UNIFORM_BUFFER)
                {
                    auto* vkUniformBuffer = dynamic_cast<VulkanUniformBuffer*>(m_UniformBuffers[currentFrame][imageInfo.name].get());
                    m_BufferInfoPool[index].buffer = vkUniformBuffer->GetBuffer();
                    m_BufferInfoPool[index].offset = imageInfo.offset;
                    m_BufferInfoPool[index].range = imageInfo.size;

                    VkWriteDescriptorSet writeDescriptorSet = {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = m_DescriptorSet[currentFrame];
                    writeDescriptorSet.descriptorType = VulkanUtility::DescriptorTypeToVK(imageInfo.type);
                    writeDescriptorSet.dstBinding = imageInfo.binding;
                    writeDescriptorSet.pBufferInfo = &m_BufferInfoPool[index];
                    writeDescriptorSet.descriptorCount = 1;

                    m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                    index++;
                    descriptorWritesCount++;

                    if(imageInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                        m_Dynamic = true;
                }
            }

            vkUpdateDescriptorSets(GET_DEVICE(), descriptorWritesCount,
                                   m_WriteDescriptorSetPool.data(), 0, nullptr);

            m_DescriptorUpdated[currentFrame] = true;
        }
    }

    void VulkanDescriptorSet::TransitionImages(CommandBuffer* commandBuffer)
    {
        for(auto &imageInfo: m_Descriptors.descriptors)
        {
            if((imageInfo.type == DescriptorType::IMAGE_SAMPLER || imageInfo.type == DescriptorType::IMAGE_STORAGE) &&
               (imageInfo.texture || imageInfo.textures))
            {
                if(imageInfo.textureCount == 1)
                {
                    if(imageInfo.texture)
                    {
                        if(imageInfo.type == DescriptorType::IMAGE_STORAGE)
                            dynamic_cast<VulkanTexture2D*>(imageInfo.texture)->TransitionImage(VK_IMAGE_LAYOUT_GENERAL,
                                                                                (VulkanCommandBuffer*) commandBuffer);
                        else
                            TransitionImageToCorrectLayout(imageInfo.texture);
                    }
                }
            }
        }
    }

    void VulkanDescriptorSet::SetTexture(const std::string &name, Texture* texture, uint32_t mipIndex,
                                         TextureType textureType)
    {
        for(auto &descriptor: m_Descriptors.descriptors)
        {
            if((descriptor.type == DescriptorType::IMAGE_SAMPLER || descriptor.type == DescriptorType::IMAGE_STORAGE) &&
               descriptor.name == name)
            {
                descriptor.texture = texture;
                descriptor.textureType = textureType;
                descriptor.textureCount = texture ? 1 : 0;
                descriptor.mipLevel = mipIndex;
                m_DescriptorDirty[0] = true;
                m_DescriptorDirty[1] = true;
                m_DescriptorDirty[2] = true;
            }
        }
    }

    void VulkanDescriptorSet::SetTexture(const std::string &name, Texture** texture, uint32_t textureCount,
                                         TextureType textureType)
    {
        for(auto &descriptor: m_Descriptors.descriptors)
        {
            if((descriptor.type == DescriptorType::IMAGE_SAMPLER || descriptor.type == DescriptorType::IMAGE_STORAGE) &&
               descriptor.name == name)
            {
                descriptor.textureCount = textureCount;
                descriptor.textures = texture;
                descriptor.textureType = textureType;

                m_DescriptorDirty[0] = true;
                m_DescriptorDirty[1] = true;
                m_DescriptorDirty[2] = true;
            }
        }
    }

    void VulkanDescriptorSet::SetBuffer(const std::string &name, UniformBuffer* buffer)
    {
        uint32_t currentFrame = gVulkanContext.GetSwapChain()->GetCurrentBufferIndex();
#if 0
        for(auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    descriptor.buffer = buffer;
                }
            }
#endif
    }

    UniformBuffer* VulkanDescriptorSet::GetUnifromBuffer(const std::string &name)
    {
        uint32_t currentFrame = gVulkanContext.GetSwapChain()->GetCurrentBufferIndex();

        // for(auto& buffers : m_UniformBuffers[currentFrame])
        //{
        // if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
        //{
        // return descriptor.buffer;
        // }
        // }

        LOG_FORMAT("Buffer not found %s", name.c_str());
        return nullptr;
    }

    void VulkanDescriptorSet::SetUniform(const std::string &bufferName, const std::string &uniformName, void* data)
    {
        auto itr = m_UniformBuffersData.find(bufferName);
        if(itr != m_UniformBuffersData.end())
        {
            for(auto &member: itr->second.m_Members)
            {
                if(member.name == uniformName)
                {
                    itr->second.LocalStorage.Write(data, member.size, member.offset);

                    itr->second.HasUpdated[0] = true;
                    itr->second.HasUpdated[1] = true;
                    itr->second.HasUpdated[2] = true;
                    return;
                }
            }
        }

        LOG_FORMAT("Uniform not found %s.%s", bufferName.c_str(), uniformName.c_str());
    }

    void VulkanDescriptorSet::SetUniform(const std::string &bufferName, const std::string &uniformName, void* data,
                                         uint32_t size)
    {
        std::map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffersData.find(bufferName);
        if(itr != m_UniformBuffersData.end())
        {
            for(auto &member: itr->second.m_Members)
            {
                if(member.name == uniformName)
                {
                    itr->second.LocalStorage.Write(data, size, member.offset);
                    itr->second.HasUpdated[0] = true;
                    itr->second.HasUpdated[1] = true;
                    itr->second.HasUpdated[2] = true;
                    return;
                }
            }
        }

        LOG_FORMAT("Uniform not found %s.%s", bufferName.c_str(), uniformName.c_str());
    }

    void VulkanDescriptorSet::SetUniformBufferData(const std::string &bufferName, void* data)
    {

        auto itr = m_UniformBuffersData.find(bufferName);
        if(itr != m_UniformBuffersData.end())
        {
            itr->second.LocalStorage.Write(data, itr->second.LocalStorage.GetSize(), 0);
            itr->second.HasUpdated[0] = true;
            itr->second.HasUpdated[1] = true;
            itr->second.HasUpdated[2] = true;
            return;
        }

        LOG_FORMAT("Uniform not found %s", bufferName.c_str());
    }

    void VulkanDescriptorSet::SetUniformDynamic(const std::string &bufferName, uint32_t size)
    {
        auto itr = m_UniformBuffersData.find(bufferName);
        if(itr != m_UniformBuffersData.end())
        {
            itr->second.LocalStorage.Allocate(size);
            for(auto &member: itr->second.m_Members)
            {
                member.size = size;
            }
        }
    }

    VkDescriptorSet VulkanDescriptorSet::GetDescriptorSet()
    {
        uint32_t currentFrame = gVulkanContext.GetSwapChain()->GetCurrentBufferIndex();
        return m_DescriptorSet[currentFrame];
    }
} // NekoEngine