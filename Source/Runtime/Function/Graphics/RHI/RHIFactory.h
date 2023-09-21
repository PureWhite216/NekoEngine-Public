#pragma once
#include "RHI/Shader.h"
#include "RHI/UniformBuffer.h"
#include "RHI/VertexBuffer.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Texture.h"
#include "RHI/RenderPass.h"
#include "RHI/Framebuffer.h"
#include "RHI/CommandBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/BufferLayout.h"

namespace NekoEngine
{
    class RHIFactory
    {
    public:
        virtual Shader* CreateShader(const String filePath) = 0;
        virtual Shader* CreateShaderFromEmbeddedArray(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize) = 0;

        virtual Texture2D* CreateTexture2D(TextureDesc parameters, uint32_t width, uint32_t height) = 0;
        virtual Texture2D* CreateTexture2DFromSource(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) = 0;
        virtual Texture2D* CreateTexture2DFromFile(const std::string& name, const std::string& filepath, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) = 0;
        virtual TextureDepth* CreateTextureDepth(uint32_t width, uint32_t height) = 0;
        virtual TextureDepthArray* CreateTextureDepthArray(uint32_t width, uint32_t height, uint32_t count) = 0;
        virtual TextureCube* CreateTextureCube(uint32_t size, void* data, bool isHDR = false) = 0;
        virtual TextureCube* CreateTextureCubeFromFile(const std::string& filepath) = 0;
        virtual TextureCube* CreateTextureCubeFromFiles(const std::string* files) = 0;
        virtual TextureCube* CreateTextureCubeFromVCross(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions) = 0;

        virtual Pipeline* CreatePipeline(const PipelineDesc& pipelineDesc) = 0;

        virtual DescriptorSet* CreateDescriptor(const DescriptorDesc& desc) = 0;

        virtual IndexBuffer* CreateIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC) = 0;
        virtual IndexBuffer* CreateIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC) = 0;

        virtual VertexBuffer* CreateVertexBuffer(const BufferUsage& usage = BufferUsage::STATIC) = 0;

        virtual RenderPass* CreateRenderPass(const RenderPassDesc& renderPassDesc) = 0;

        virtual CommandBuffer* CreateCommandBuffer() = 0;
    };


} // NekoEngine

