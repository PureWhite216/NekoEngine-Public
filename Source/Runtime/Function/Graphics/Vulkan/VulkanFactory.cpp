#include "VulkanFactory.h"
#include "VulkanShader.h"
#include "File/VirtualFileSystem.h"
#include "VulkanTexture.h"
#include "VulkanDescriptorSet.h"
#include "VulkanIndexBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanSwapChain.h"
#include "VulkanVertexBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"

namespace NekoEngine
{
    Shader* VulkanFactory::CreateShader(const String filePath)
    {
        std::string physicalPath;
        VirtualFileSystem::ResolvePhysicalPath(filePath, physicalPath, false);
        return new VulkanShader(physicalPath);
    }

    Shader* VulkanFactory::CreateShaderFromEmbeddedArray(const uint32_t* vertData, uint32_t vertDataSize,
                                                         const uint32_t* fragData, uint32_t fragDataSize)
    {
        return new VulkanShader(vertData, vertDataSize, fragData, fragDataSize);
    }

    Texture2D* VulkanFactory::CreateTexture2D(TextureDesc parameters, uint32_t width, uint32_t height)
    {
        return new VulkanTexture2D(parameters, width, height);
    }

    Texture2D*
    VulkanFactory::CreateTexture2DFromSource(uint32_t width, uint32_t height, void* data, TextureDesc parameters,
                                             TextureLoadOptions loadOptions)
    {
        return new VulkanTexture2D(width, height, data, parameters, loadOptions);
    }

    Texture2D*
    VulkanFactory::CreateTexture2DFromFile(const std::string &name, const std::string &filepath, TextureDesc parameters,
                                           TextureLoadOptions loadOptions)
    {
        return new VulkanTexture2D(name, filepath, parameters, loadOptions);
    }

    TextureDepth* VulkanFactory::CreateTextureDepth(uint32_t width, uint32_t height)
    {
        return new VulkanTextureDepth(width, height);
    }

    TextureDepthArray* VulkanFactory::CreateTextureDepthArray(uint32_t width, uint32_t height, uint32_t count)
    {
        return new VulkanTextureDepthArray(width, height, count);
    }

    TextureCube* VulkanFactory::CreateTextureCube(uint32_t size, void* data, bool isHDR)
    {
        return new VulkanTextureCube(size, data, isHDR);
    }

    TextureCube* VulkanFactory::CreateTextureCubeFromFile(const std::string &filepath)
    {
        return new VulkanTextureCube(filepath);
    }

    TextureCube* VulkanFactory::CreateTextureCubeFromFiles(const std::string* files)
    {
        return new VulkanTextureCube(files);
    }

    TextureCube* VulkanFactory::CreateTextureCubeFromVCross(const std::string* files, uint32_t mips, TextureDesc params,
                                                            TextureLoadOptions loadOptions)
    {
        return new VulkanTextureCube(files, mips, params, loadOptions);
    }

    DescriptorSet* VulkanFactory::CreateDescriptor(const DescriptorDesc& desc)
    {
        return new VulkanDescriptorSet(desc);
    }

    IndexBuffer* VulkanFactory::CreateIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage)
    {
        return new VulkanIndexBuffer(data, count, bufferUsage);
    }

    IndexBuffer* VulkanFactory::CreateIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage)
    {
        return new VulkanIndexBuffer(data, count, bufferUsage);
    }

    Pipeline* VulkanFactory::CreatePipeline(const PipelineDesc &pipelineDesc)
    {
        return new VulkanPipeline(pipelineDesc);
    }

    VertexBuffer* VulkanFactory::CreateVertexBuffer(const BufferUsage& usage)
    {
        return new VulkanVertexBuffer(usage);
    }

    RenderPass* VulkanFactory::CreateRenderPass(const RenderPassDesc& renderPassDesc)
    {
        return new VulkanRenderPass(renderPassDesc);
    }

    CommandBuffer* VulkanFactory::CreateCommandBuffer()
    {
        return new VulkanCommandBuffer();
    }

} // NekoEngine