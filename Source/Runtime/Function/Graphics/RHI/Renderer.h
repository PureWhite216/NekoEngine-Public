#pragma once
#include "Core.h"
#include "Asset/AssetManager.h"
#include "Renderable/Mesh.h"
#include "SwapChain.h"
//#include "RHIFactory.h"

namespace NekoEngine
{
    struct RenderAPICapabilities
    {
        std::string Vendor;
        std::string Renderer;
        std::string Version;

        int MaxSamples                   = 0;
        float MaxAnisotropy              = 0.0f;
        int MaxTextureUnits              = 0;
        int UniformBufferOffsetAlignment = 0;
        bool WideLines                   = false;
        bool SupportCompute              = false;
    };

    struct RenderConfig
    {
        uint32_t IrradianceMapSize  = 64;
        uint32_t EnvironmentMapSize = 1024;
    };

    class RHIFactory;

    class Renderer
    {
    public:
        static RenderAPICapabilities capabilities;
    protected:
        SharedPtr<RHIFactory> rhiFactory;
        SharedPtr<ShaderLibrary> shaderLibrary;
        RenderConfig renderConfig;
    public:
        Renderer() = default;
        virtual ~Renderer() = default;

        virtual void Init(bool loadEmbeddedShaders = false) = 0;
        virtual void LoadEngineShaders(bool loadEmbeddedShaders = false);
        virtual void Begin() = 0;
        virtual void Draw(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType = DataType::UNSIGNED_INT, void* indices = nullptr) = 0;
        virtual void DrawMesh(CommandBuffer* commandBuffer, Pipeline* pipeline, Mesh* mesh);
        virtual void DrawIndexed(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) = 0;
        virtual void Dispatch(CommandBuffer* commandBuffer, uint32_t workGroupSizeX, uint32_t workGroupSizeY, uint32_t workGroupSizeZ) = 0;
        virtual void ClearRenderTarget(Texture* texture, CommandBuffer* commandBuffer, Color clearColour = Color(0.1f, 0.1f, 0.1f, 1.0f)){};
        virtual void Present() = 0;
        virtual void OnResize(uint32_t height, uint32_t width) = 0;
        virtual void BindDescriptorSets(Pipeline* pipeline, CommandBuffer* commandBuffer, uint32_t dynamicOffset, DescriptorSet** descriptorSets, uint32_t descriptorCount) = 0;
        virtual void SaveScreenshot(const std::string& path, Texture* texture = nullptr) {};
//        virtual SwapChain* GetSwapChain() const = 0;
        virtual GraphicsContext* GetGraphicsContext() const = 0;
        virtual uint32_t GetGPUCount() const { return 1; }

        SharedPtr<RHIFactory> GetRHIFactory() const { return rhiFactory; }
        SharedPtr<ShaderLibrary> GetShaderLibrary() const { return shaderLibrary; }

        const RenderConfig& GetRenderConfig() const
        {
            return renderConfig;
        }
    };
}
