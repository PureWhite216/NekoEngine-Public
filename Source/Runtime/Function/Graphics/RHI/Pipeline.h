#pragma once
#include "Renderer.h"
#include "Definitions.h"
#include "Shader.h"
namespace NekoEngine
{
    struct PipelineAsset
    {
        SharedPtr<Pipeline> pipeline;
        float timeSinceLastAccessed;
    };

    struct PipelineDesc
    {
        SharedPtr<Shader> shader;

        CullMode cullMode       = CullMode::BACK;
        PolygonMode polygonMode = PolygonMode::FILL;
        DrawType drawType       = DrawType::TRIANGLE;
        BlendMode blendMode     = BlendMode::None;

        bool isTransparencyEnabled = false;
        bool isDepthBiasEnabled    = false;
        bool isSwapChainTarget     = false;
        bool isClearTargets        = false;

        Array<Texture*, MAX_RENDER_TARGETS> colourTargets = {};

        Texture* cubeMapTarget        = nullptr;
        Texture* depthTarget          = nullptr;
        Texture* depthArrayTarget     = nullptr;
        Color clearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
        float lineWidth               = 1.0f;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor    = 0.0f;
        int cubeMapIndex              = 0;
        int mipIndex                  = 0;

        std::string DebugName;
    };

    class Pipeline
    {
    public:
        static SharedPtr<Pipeline> Get(const PipelineDesc& pipelineDesc);
        static void ClearCache();
        static void DeleteUnusedCache();

        Pipeline() = default;
        virtual ~Pipeline() = default;

        virtual void Bind(CommandBuffer* commandBuffer, uint32_t layer = 0) = 0;
        virtual void End(CommandBuffer* commandBuffer) { }
        virtual void ClearRenderTargets(CommandBuffer* commandBuffer) { }
        virtual SharedPtr<Shader> GetShader() const = 0;

        uint32_t GetWidth();
        uint32_t GetHeight();
        bool IsCompute() const { return isCompute; }
    protected:
        PipelineDesc pipelineDesc;
        SharedPtr<Shader> shader;
        SharedPtr<RenderPass> renderPass;
        bool isDepthBiasEnabled = false;
        bool isCompute = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
    };

} // NekoEngine
