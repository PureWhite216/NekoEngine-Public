#pragma once
#include "RHI/RHIFactory.h"
namespace NekoEngine
{
    class VulkanFactory : public RHIFactory
    {
    public:
        Shader* CreateShader(const String filePath) override;
        Shader* CreateShaderFromEmbeddedArray(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize) override;

        Texture2D* CreateTexture2D(TextureDesc parameters, uint32_t width, uint32_t height) override;
        Texture2D* CreateTexture2DFromSource(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) override;
        Texture2D* CreateTexture2DFromFile(const std::string& name, const std::string& filepath, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) override;
        TextureDepth* CreateTextureDepth(uint32_t width, uint32_t height) override;
        TextureDepthArray* CreateTextureDepthArray(uint32_t width, uint32_t height, uint32_t count) override;
        TextureCube* CreateTextureCube(uint32_t size, void* data, bool isHDR) override;
        TextureCube* CreateTextureCubeFromFile(const std::string &filepath) override;
        TextureCube* CreateTextureCubeFromFiles(const std::string* files) override;
        TextureCube* CreateTextureCubeFromVCross(const std::string* files, uint32_t mips, TextureDesc params,
                                                 TextureLoadOptions loadOptions) override;

        DescriptorSet* CreateDescriptor(const DescriptorDesc& desc) override;

        IndexBuffer* CreateIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC) override;
        IndexBuffer* CreateIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC) override;

        Pipeline* CreatePipeline(const PipelineDesc& pipelineDesc) override;

        VertexBuffer* CreateVertexBuffer(const BufferUsage& usage = BufferUsage::STATIC) override;

        RenderPass* CreateRenderPass(const RenderPassDesc& renderPassDesc) override;

        CommandBuffer* CreateCommandBuffer() override;
    };

} // NekoEngine

