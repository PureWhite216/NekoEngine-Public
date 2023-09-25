#include <glm/gtx/quaternion.hpp>
#include "SceneRenderer.h"
#include "Maths.h"
#include "Renderable/MeshFactory.h"
#include "Memory/Memory.h"
#include "Renderable/Environment.h"
#include "Entity/Entity.h"
#include "Component/ModelComponent.h"
#include "GUI/ImGuiUtility.h"
#include "RHI/Renderer.h"
#include "Engine.h"

namespace NekoEngine
{
    static const uint32_t MaxPoints = 10000;
    static const uint32_t MaxPointVertices = MaxPoints * 4;
    static const uint32_t MaxPointIndices = MaxPoints * 6;
    static const uint32_t MAX_BATCH_DRAW_CALLS = 1000;
    static const uint32_t RENDERER_POINT_SIZE = sizeof(PointVertexData) * 4;
    static const uint32_t RENDERER_POINT_BUFFER_SIZE = RENDERER_POINT_SIZE * MaxPointVertices;

    static const uint32_t MaxLines = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices = MaxLines * 6;
    static const uint32_t MAX_LINE_BATCH_DRAW_CALLS = 100;
    static const uint32_t RENDERER_LINE_SIZE = sizeof(LineVertexData) * 4;
    static const uint32_t RENDERER_LINE_BUFFER_SIZE = RENDERER_LINE_SIZE * MaxLineVertices;

} // NekoEngine

namespace NekoEngine
{
    SceneRenderer::SceneRenderer(uint32_t width, uint32_t height)
    {
        m_CubeMap = nullptr;
        m_ClearColour = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        m_SupportCompute = gEngine->GetRenderer()->capabilities.SupportCompute;

        TextureDesc mainRenderTargetDesc;
        mainRenderTargetDesc.format = RHIFormat::R11G11B10_Float;
        mainRenderTargetDesc.flags = TextureFlags::Texture_RenderTarget;
        mainRenderTargetDesc.wrap = TextureWrap::CLAMP_TO_EDGE;
        mainRenderTargetDesc.minFilter = TextureFilter::LINEAR;
        mainRenderTargetDesc.magFilter = TextureFilter::LINEAR;
        mainRenderTargetDesc.generateMipMaps = false;
        m_MainTexture = GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, width, height);
        m_PostProcessTexture1 = GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, width, height);

        // Setup shadow pass data
        m_ShadowData.m_ShadowTex = nullptr;
        m_ShadowData.m_ShadowMapNum = 4;
        m_ShadowData.m_ShadowMapSize = 1024;
        m_ShadowData.m_ShadowMapsInvalidated = true;
        m_ShadowData.m_CascadeSplitLambda = 0.92f;
        m_ShadowData.m_Shader = GET_SHADER_LIB()->GetResource("Shadow");
        m_ShadowData.m_ShadowTex = GET_RHI_FACTORY()->CreateTextureDepthArray(m_ShadowData.m_ShadowMapSize,
                                                                              m_ShadowData.m_ShadowMapSize,
                                                                              m_ShadowData.m_ShadowMapNum);
        m_ShadowData.m_LightSize = 1.5f;
        m_ShadowData.m_MaxShadowDistance = 500.0f;
        m_ShadowData.m_ShadowFade = 40.0f;
        m_ShadowData.m_CascadeFade = 3.0f;
        m_ShadowData.m_InitialBias = 0.00f;

        DescriptorDesc descriptorDesc{};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ShadowData.m_Shader.get();
        m_ShadowData.m_DescriptorSet.resize(1);
        m_ShadowData.m_DescriptorSet[0] = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        m_ShadowData.m_CurrentDescriptorSets.resize(2);

        m_ShadowData.m_CascadeCommandQueue[0].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[1].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[2].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[3].reserve(1000);

        // Setup forward pass data
        m_ForwardData.m_DepthTest = true;
        m_ForwardData.m_Shader = GET_SHADER_LIB()->GetResource("ForwardPBR");
        m_ForwardData.m_DepthTexture = GET_RHI_FACTORY()->CreateTextureDepth(width, height);
        m_ForwardData.m_CommandQueue.reserve(1000);

        const size_t minUboAlignment = size_t(gEngine->GetRenderer()->capabilities.UniformBufferOffsetAlignment);

        m_ForwardData.m_DynamicAlignment = sizeof(glm::mat4);
        if(minUboAlignment > 0)
        {
            m_ForwardData.m_DynamicAlignment =
                    (m_ForwardData.m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        m_ForwardData.m_TransformData = static_cast<glm::mat4*>(Memory::AlignedAlloc(
                static_cast<uint32_t>(MAX_OBJECTS * m_ForwardData.m_DynamicAlignment),
                m_ForwardData.m_DynamicAlignment));

        const int SSAO_NOISE_DIM = 4;
        std::vector<glm::vec4> noiseData(SSAO_NOISE_DIM * SSAO_NOISE_DIM);

        for(glm::vec4 &noiseSample: noiseData)
        {
            // Random rotations around z-axis
            noiseSample = glm::vec4(Random32::Rand(-1.0f, 1.0f), Random32::Rand(-1.0f, 1.0f), 0.0f, 0.0f);
        }

        TextureDesc noiseTextureDesc;
        noiseTextureDesc.format = RHIFormat::R32G32B32A32_Float;
        noiseTextureDesc.wrap = TextureWrap::REPEAT;
        noiseTextureDesc.minFilter = TextureFilter::NEAREST;
        noiseTextureDesc.magFilter = TextureFilter::NEAREST;
        noiseTextureDesc.generateMipMaps = false;
        noiseTextureDesc.anisotropicFiltering = false;
        noiseTextureDesc.flags = 0;
        m_NoiseTexture = GET_RHI_FACTORY()->CreateTexture2DFromSource(SSAO_NOISE_DIM, SSAO_NOISE_DIM,
                                                                      (void*) noiseData.data(), noiseTextureDesc);
        noiseTextureDesc.flags = TextureFlags::Texture_RenderTarget;
        noiseTextureDesc.wrap = TextureWrap::CLAMP_TO_EDGE;

        noiseTextureDesc.minFilter = TextureFilter::LINEAR;
        noiseTextureDesc.magFilter = TextureFilter::LINEAR;
        m_NormalTexture = GET_RHI_FACTORY()->CreateTexture2D(noiseTextureDesc, width, height);

        m_SSAOTexture = GET_RHI_FACTORY()->CreateTexture2D(noiseTextureDesc, width / 2, height / 2);
        m_SSAOTexture1 = GET_RHI_FACTORY()->CreateTexture2D(noiseTextureDesc, width / 2, height / 2);

        m_ForwardData.m_BiasMatrix = glm::mat4(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.5, 0.5, 0.0, 1.0);

        TextureDesc param;
        param.minFilter = TextureFilter::LINEAR;
        param.magFilter = TextureFilter::LINEAR;
        param.format = RHIFormat::R16G16_Float;
        param.srgb = false;
        param.wrap = TextureWrap::CLAMP_TO_EDGE;
        param.flags = TextureFlags::Texture_RenderTarget;
        param.generateMipMaps = false;
        m_ForwardData.m_BRDFLUT = UniquePtr<Texture2D>(
                GET_RHI_FACTORY()->CreateTexture2D(param, BRDFTextureWidth, BRDFTextureHeight));

        m_GenerateBRDFLUT = true;

        auto descriptorSetScene = m_ForwardData.m_Shader->GetDescriptorInfo(2);
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ForwardData.m_Shader.get();
        m_ForwardData.m_DescriptorSet.resize(3);
        m_ForwardData.m_DescriptorSet[0] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        descriptorDesc.layoutIndex = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        // m_ForwardData.m_DescriptorSet[0]->SetUniformDynamic("TransformData", static_cast<uint32_t>(MAX_OBJECTS * m_ForwardData.m_DynamicAlignment));

        m_ForwardData.m_DefaultMaterial = new Material(m_ForwardData.m_Shader);
        uint32_t blackCubeTextureData[6] = {0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000};
        m_DefaultTextureCube = GET_RHI_FACTORY()->CreateTextureCube(1, blackCubeTextureData);

        MaterialProperties properties;
        properties.albedoColour = glm::vec4(1.0f);
        properties.roughness = 0.5f;
        properties.metallic = 0.5f;
        properties.albedoMapFactor = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor = 0.0f;
        properties.metallicMapFactor = 0.0f;

        m_ForwardData.m_DefaultMaterial->SetMaterialProperites(properties);
        // m_ForwardData.m_DefaultMaterial->CreateDescriptorSet(1);

        m_ForwardData.m_CurrentDescriptorSets.resize(3);

        // Set up skybox pass data
        m_ScreenQuad = MeshFactory::CreateQuad();
        m_SkyboxShader = GET_SHADER_LIB()->GetResource("Skybox");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_SkyboxShader.get();
        m_SkyboxDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        // Set up final pass data
        m_FinalPassShader = GET_SHADER_LIB()->GetResource("FinalPass");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_FinalPassShader.get();
        m_FinalPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        // PostProcesses

        m_ToneMappingPassShader = GET_SHADER_LIB()->GetResource("ToneMapping");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ToneMappingPassShader.get();
        m_ToneMappingPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_DepthOfFieldShader = GET_SHADER_LIB()->GetResource("DepthOfField");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DepthOfFieldShader.get();
        m_DepthOfFieldPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_BloomPassShader = GET_SHADER_LIB()->GetResource(m_SupportCompute ? "BloomComp" : "Bloom");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_BloomPassShader.get();

        if(m_BloomPassShader->IsCompiled())
        {
            for(int i = 0; i < 6; i++)
                m_BloomDescriptorSets.push_back(
                        SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc)));
        }

        mainRenderTargetDesc.flags =
                TextureFlags::Texture_RenderTarget | TextureFlags::Texture_CreateMips | TextureFlags::Texture_MipViews;
        m_BloomTexture = GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, width, height);
        m_BloomTexture1 = GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, width, height);
        m_BloomTexture2 = GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, width, height);

        m_FXAAShader = GET_SHADER_LIB()->GetResource(m_SupportCompute ? "FXAAComp" : "FXAA");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_FXAAShader.get();
        m_FXAAPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_DebandingShader = GET_SHADER_LIB()->GetResource("Debanding");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebandingShader.get();
        m_DebandingPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_ChromaticAberationShader = GET_SHADER_LIB()->GetResource("ChromaticAberation");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ChromaticAberationShader.get();
        m_ChromaticAberationPassDescriptorSet = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_DepthPrePassShader = GET_SHADER_LIB()->GetResource("DepthPrePass");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DepthPrePassShader.get();
        m_DepthPrePassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_FilmicGrainShader = GET_SHADER_LIB()->GetResource("FilmicGrain");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_FilmicGrainShader.get();
        m_FilmicGrainPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_SSAOShader = GET_SHADER_LIB()->GetResource("SSAO");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_SSAOShader.get();

        if(m_SSAOShader->IsCompiled())
            m_SSAOPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_SSAOBlurShader = GET_SHADER_LIB()->GetResource("SSAOBlur");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_SSAOBlurShader.get();
        if(m_SSAOBlurShader->IsCompiled())
        {
            m_SSAOBlurPassDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
            m_SSAOBlurPassDescriptorSet2 = SharedPtr<DescriptorSet>(
                    GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        // m_OutlineShader = GET_SHADER_LIB()->GetResource("Outline");
        //         descriptorDesc.layoutIndex = 0;
        //         descriptorDesc.shader      = m_OutlineShader.get();
        //         m_OutlinePassDescriptorSet    = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        // Setup 2D pass data
        m_Renderer2DData.m_IndexCount = 0;
        m_Renderer2DData.m_Buffer = nullptr;
        m_Renderer2DData.m_RenderToDepthTexture = true;
        m_Renderer2DData.m_TriangleIndicies = false;
        m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_Renderer2DData.m_Limits.MaxTextures = 16; // gEngine->GetRenderer()->capabilities.MaxTextureUnits;

        m_Renderer2DData.m_Shader = GET_SHADER_LIB()->GetResource("Batch2D");

        m_Renderer2DData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_Renderer2DData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
        m_Renderer2DData.m_DescriptorSet.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_Renderer2DData.m_PreviousFrameTextureCount.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_Renderer2DData.m_PreviousFrameTextureCount[i] = 0;
            m_Renderer2DData.m_DescriptorSet[i].resize(2);
            if(i == 0)
            {
                descriptorDesc.layoutIndex = 0;
                m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<DescriptorSet>(
                        GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
            }
            descriptorDesc.layoutIndex = 1;
            m_Renderer2DData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        m_Renderer2DData.m_VertexBuffers.resize(3);

        uint32_t* indices = new uint32_t[m_Renderer2DData.m_Limits.IndiciesSize];

        if(m_Renderer2DData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i] = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_Renderer2DData.m_IndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices,
                                                                              m_Renderer2DData.m_Limits.IndiciesSize);
        m_2DBufferBase.resize(3);

        for(int i = 0; i < GET_SWAP_CHAIN()->GetSwapChainBufferCount(); i++)
        {
            m_QuadBufferBase.push_back(new VertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);

            // m_2DBufferBase.push_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads  * 4]);
            TextVertexBufferBase.push_back(new TextVertexData[m_TextRendererData.m_Limits.MaxQuads * 4]);
            m_LineBufferBase.push_back(new LineVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
            m_PointBufferBase.push_back(new PointVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
            DebugTextVertexBufferBase.push_back(new TextVertexData[m_DebugTextRendererData.m_Limits.MaxQuads * 4]);
        }

        delete[] indices;

        m_Renderer2DData.m_CurrentDescriptorSets.resize(2);

        // Setup text pass
        m_TextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_TextRendererData.m_RenderToDepthTexture = true;
        m_TextRendererData.m_TriangleIndicies = false;
        m_TextRendererData.m_Limits.SetMaxQuads(10000);
        m_TextRendererData.m_Limits.MaxTextures = 16; // gEngine->GetRenderer()->capabilities.MaxTextureUnits;

        TextVertexBufferPtr = TextVertexBufferBase[0];

        m_TextRendererData.m_Shader = GET_SHADER_LIB()->GetResource("Text");

        m_TextRendererData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_TextRendererData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
        m_TextRendererData.m_DescriptorSet.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_TextRendererData.m_PreviousFrameTextureCount.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_TextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_TextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_TextRendererData.m_DescriptorSet[i].resize(2);
            if(i == 0)
            {
                descriptorDesc.layoutIndex = 0;
                m_TextRendererData.m_DescriptorSet[0][0] = SharedPtr<DescriptorSet>(
                        GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
            }
            descriptorDesc.layoutIndex = 1;
            m_TextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        m_TextRendererData.m_VertexBuffers.resize(GET_SWAP_CHAIN()->GetSwapChainBufferCount());
        indices = new uint32_t[m_TextRendererData.m_Limits.IndiciesSize];

        if(m_TextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i] = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_TextRendererData.m_IndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices,
                                                                                m_TextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        m_TextRendererData.m_CurrentDescriptorSets.resize(2);

        // Debug Render

        // Points
        m_DebugDrawData.m_PointShader = GET_SHADER_LIB()->GetResource("Batch2DPoint");

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_PointShader.get();
        m_DebugDrawData.m_PointDescriptorSet.resize(1);
        m_DebugDrawData.m_PointDescriptorSet[0] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        indices = new uint32_t[MaxPointIndices];

        int32_t offset = 0;
        for(int32_t i = 0; i < MaxPointIndices; i += 6)
        {
            indices[i] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        m_DebugDrawData.m_PointIndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices, MaxPointIndices);
        delete[] indices;

        // Lines
        m_DebugDrawData.m_LineShader = GET_SHADER_LIB()->GetResource("Batch2DLine");
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_LineShader.get();
        m_DebugDrawData.m_LineDescriptorSet.resize(1);
        m_DebugDrawData.m_LineDescriptorSet[0] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        indices = new uint32_t[MaxLineIndices];

        for(int32_t i = 0; i < MaxLineIndices; i++)
        {
            indices[i] = i;
        }

        m_DebugDrawData.m_LineIndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices, MaxLineIndices);
        delete[] indices;

        // Debug quads
        m_DebugDrawData.m_Renderer2DData.m_IndexCount = 0;
        m_DebugDrawData.m_Renderer2DData.m_Buffer = nullptr;
        m_DebugDrawData.m_Renderer2DData.m_RenderToDepthTexture = true;
        m_DebugDrawData.m_Renderer2DData.m_TriangleIndicies = false;
        m_DebugDrawData.m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_DebugDrawData.m_Renderer2DData.m_Shader = GET_SHADER_LIB()->GetResource("Batch2D");

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader.get();
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet.resize(1);

        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0].resize(2);
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        descriptorDesc.layoutIndex = 1;
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1] = SharedPtr<DescriptorSet>(
                GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

        m_DebugDrawData.m_Renderer2DData.m_VertexBuffers.resize(3);

        indices = new uint32_t[m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize];

        {
            for(uint32_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        m_DebugDrawData.m_Renderer2DData.m_IndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices,
                                                                                              m_Renderer2DData.m_Limits.IndiciesSize);

        delete[] indices;

        m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets.resize(2);

        // Setup debug text pass
        m_DebugTextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_DebugTextRendererData.m_RenderToDepthTexture = true;
        m_DebugTextRendererData.m_TriangleIndicies = false;
        m_DebugTextRendererData.m_Limits.SetMaxQuads(10000);
        m_DebugTextRendererData.m_Limits.MaxTextures = 16; // gEngine->GetRenderer()->capabilities.MaxTextureUnits;

        DebugTextVertexBufferPtr = DebugTextVertexBufferBase[0];

        m_DebugTextRendererData.m_Shader = GET_SHADER_LIB()->GetResource("Text");

        m_DebugTextRendererData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_DebugTextRendererData.m_TransformationBack = &m_DebugTextRendererData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
        m_DebugTextRendererData.m_DescriptorSet.resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);
        m_DebugTextRendererData.m_PreviousFrameTextureCount.resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_DebugTextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_DebugTextRendererData.m_DescriptorSet[i].resize(2);
            // if (i == 0)
            {
                descriptorDesc.layoutIndex = 0;
                m_DebugTextRendererData.m_DescriptorSet[i][0] = SharedPtr<DescriptorSet>(
                        GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
            }
            descriptorDesc.layoutIndex = 1;
            m_DebugTextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        m_DebugTextRendererData.m_VertexBuffers.resize(GET_SWAP_CHAIN()->GetSwapChainBufferCount());
        indices = new uint32_t[m_DebugTextRendererData.m_Limits.IndiciesSize];

        if(m_DebugTextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i] = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_DebugTextRendererData.m_IndexBuffer = GET_RHI_FACTORY()->CreateIndexBuffer(indices,
                                                                                     m_DebugTextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        m_DebugTextRendererData.m_CurrentDescriptorSets.resize(2);
    }

    SceneRenderer::~SceneRenderer()
    {
        delete m_ForwardData.m_DepthTexture;
        delete m_MainTexture;
        delete m_PostProcessTexture1;
        delete m_SSAOTexture;
        delete m_SSAOTexture1;
        delete m_NoiseTexture;
        delete m_BloomTexture;
        delete m_BloomTexture1;
        delete m_BloomTexture2;
        delete m_NormalTexture;

        delete m_ShadowData.m_ShadowTex;
        delete m_ForwardData.m_DefaultMaterial;
        delete m_DefaultTextureCube;
        delete m_ScreenQuad;

        delete m_Renderer2DData.m_IndexBuffer;
        delete m_TextRendererData.m_IndexBuffer;
        delete m_DebugTextRendererData.m_IndexBuffer;
        delete m_DebugDrawData.m_Renderer2DData.m_IndexBuffer;
        delete m_DebugDrawData.m_LineIndexBuffer;
        delete m_DebugDrawData.m_PointIndexBuffer;

        for(auto data: TextVertexBufferBase)
            delete[] data;
        for(auto data: DebugTextVertexBufferBase)
            delete[] data;

        for(auto data: m_LineBufferBase)
            delete[] data;
        for(auto data: m_PointBufferBase)
            delete[] data;
        for(auto data: m_QuadBufferBase)
            delete[] data;

        for(int j = 0; j < GET_SWAP_CHAIN()->GetSwapChainBufferCount(); j++)
        {
            for(size_t i = 0; i < m_Renderer2DData.m_VertexBuffers[j].size(); i++)
            {
                delete m_Renderer2DData.m_VertexBuffers[j][i];
            }

            for(uint32_t i = 0; i < m_TextRendererData.m_VertexBuffers[j].size(); i++)
            {
                delete m_TextRendererData.m_VertexBuffers[j][i];
            }

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_VertexBuffers[j].size(); i++)
            {
                delete m_DebugTextRendererData.m_VertexBuffers[j][i];
            }

            for(size_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j].size(); i++)
            {
                delete m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j][i];
            }

            for(auto data: m_2DBufferBase[j])
                delete[] data;
        }

        for(int i = 0; i < m_DebugDrawData.m_PointVertexBuffers.size(); i++)
        {
            delete m_DebugDrawData.m_PointVertexBuffers[i];
        }

        for(int i = 0; i < m_DebugDrawData.m_LineVertexBuffers.size(); i++)
        {
            delete m_DebugDrawData.m_LineVertexBuffers[i];
        }

        DebugRenderer::Release();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {


        width -= (width % 2 != 0) ? 1 : 0;
        height -= (height % 2 != 0) ? 1 : 0;

        m_ForwardData.m_DepthTexture->Resize(width, height);
        m_MainTexture->Resize(width, height);
        m_PostProcessTexture1->Resize(width, height);
        m_SSAOTexture->Resize(width / 2, height / 2);
        m_SSAOTexture1->Resize(width / 2, height / 2);
        m_BloomTexture->Resize(width, height);
        m_BloomTexture1->Resize(width, height);
        m_BloomTexture2->Resize(width, height);

        m_NormalTexture->Resize(width, height);
    }

    void SceneRenderer::EnableDebugRenderer(bool enable)
    {
        m_DebugRenderEnabled = enable;

        if(m_DebugRenderEnabled)
            DebugRenderer::Init();
        else
            DebugRenderer::Release();
    }

    void SceneRenderer::BeginScene(Level* level)
    {

        auto &registry = level->GetRegistry();
        m_CurrentScene = level;
        m_Stats.FramesPerSecond = 0;
        m_Stats.NumDrawCalls = 0;
        m_Stats.NumRenderedObjects = 0;
        m_Stats.NumShadowObjects = 0;
        m_Stats.UpdatesPerSecond = 0;

        m_Renderer2DData.m_BatchDrawCallIndex = 0;
        m_TextRendererData.m_BatchDrawCallIndex = 0;
        m_DebugTextRendererData.m_BatchDrawCallIndex = 0;

        if(m_OverrideCamera)
        {
            m_Camera = m_OverrideCamera;
            m_CameraTransform = m_OverrideCameraTransform;
        }
        else
        {
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                m_Camera = &cameraView.get<Camera>(cameraView.front());
                m_CameraTransform = registry.try_get<Transform>(cameraView.front());
            }
        }

        if(!m_Camera || !m_CameraTransform)
            return;

        m_Exposure = m_Camera->GetExposure();
        m_ToneMapIndex = level->GetSettings().renderSettings.m_ToneMapIndex;

        auto view = glm::inverse(m_CameraTransform->GetWorldMatrix());
        auto proj = m_Camera->GetProjectionMatrix();
        auto projView = proj * view;

        if(m_DebugRenderEnabled)
        {
            DebugRenderer::GetInstance()->SetDimensions(m_MainTexture->GetWidth(), m_MainTexture->GetHeight());
            DebugRenderer::GetInstance()->SetProjView(projView);
        }

        auto& renderSettings = level->GetSettings().renderSettings;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[0]->SetUniform("UBO", "projView", &projView);
            m_ForwardData.m_DescriptorSet[0]->Update();
        }

        if(renderSettings.SkyboxRenderEnabled || renderSettings.Renderer3DEnabled)
        {
            auto envView = registry.view<Environment>();

            if(envView.size() == 0)
            {
                if(m_ForwardData.m_EnvironmentMap != m_DefaultTextureCube)
                {
                    m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
                    m_ForwardData.m_IrradianceMap = m_DefaultTextureCube;

                    // TODO: remove need for this
                    DescriptorDesc info{};
                    info.shader = m_ForwardData.m_Shader.get();
                    info.layoutIndex = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<DescriptorSet>(
                            GET_RHI_FACTORY()->CreateDescriptor(info));

                    m_CubeMap = nullptr;
                    DescriptorDesc descriptorDesc{};
                    descriptorDesc.layoutIndex = 0;
                    descriptorDesc.shader = m_SkyboxShader.get();
                    m_SkyboxDescriptorSet = SharedPtr<DescriptorSet>(
                            GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
                }
            }
            else
            {
                // Just use first
                const auto &env = envView.get<Environment>(envView.front());

                if(m_ForwardData.m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    DescriptorDesc info = {};
                    info.shader = m_ForwardData.m_Shader.get();
                    info.layoutIndex = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<DescriptorSet>(
                            GET_RHI_FACTORY()->CreateDescriptor(info));
                    m_ForwardData.m_EnvironmentMap = env.GetEnvironmentMap();
                    m_ForwardData.m_IrradianceMap = env.GetIrradianceMap();
                    const bool viewIrradiance = false;

                    m_CubeMap = viewIrradiance ? env.GetIrradianceMap() : env.GetEnvironmentMap();
                }
            }

            auto invProj = glm::inverse(proj);
            auto invView = glm::inverse(view);

            m_SkyboxDescriptorSet->SetUniform("UBO", "invProjection", &invProj);
            m_SkyboxDescriptorSet->SetUniform("UBO", "invView", &invView);
        }

        Light* directionaLight = nullptr;
        static Light lights[256];
        uint32_t numLights = 0;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_Frustum = m_Camera->GetFrustum(view);
            {
                auto group = registry.group<Light>(entt::get<Transform>);

                for(auto &lightEntity: group)
                {
                    if(!Entity(lightEntity, level).Active())
                        continue;

                    if(numLights >= 64)
                        break;

                    const auto &[light, trans] = group.get<Light, Transform>(lightEntity);
                    light.Position = glm::vec4(trans.GetWorldPosition(), 1.0f);
                    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
                    forward = trans.GetWorldOrientation() * forward;
                    forward = glm::normalize(forward);
                    light.Direction = glm::vec4(forward, 1.0f);

                    if(light.Type == (float) LightType::DirectionalLight)
                        directionaLight = &light;

                    if(light.Type != float(LightType::DirectionalLight))
                    {
                        auto inside = m_ForwardData.m_Frustum.IsInside(
                                BoundingSphere(glm::vec3(light.Position), light.Radius * 100));

                        if(inside == Intersection::OUTSIDE)
                            continue;
                    }

                    lights[numLights] = light;
                    lights[numLights].Intensity *= m_Exposure;
                    numLights++;
                }
            }

            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "lights", lights, sizeof(Light) * numLights);

            glm::vec4 cameraPos = glm::vec4(m_CameraTransform->GetWorldPosition(), 1.0f);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "cameraPosition", &cameraPos);
        }

        if(renderSettings.ShadowsEnabled)
        {
            for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
            {
                m_ShadowData.m_CascadeCommandQueue[i].clear();
            }

            if(directionaLight)
            {
                UpdateCascades(level, directionaLight);

                for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                {
                    m_ShadowData.m_CascadeFrustums[i].Define(m_ShadowData.m_ShadowProjView[i]);
                }
            }
        }

        m_ForwardData.m_CommandQueue.clear();

        auto &shadowData = GetShadowData();
        glm::mat4* shadowTransforms = shadowData.m_ShadowProjView;
        glm::vec4* uSplitDepth = shadowData.m_SplitDepth;
        glm::mat4 LightView = shadowData.m_LightMatrix;
        float bias = shadowData.m_InitialBias;
        float MaxShadowDist = shadowData.m_MaxShadowDistance;
        float LightSize = (float) shadowData.m_ShadowMapSize;
        float transitionFade = shadowData.m_CascadeFade;
        float ShadowFade = shadowData.m_ShadowFade;
        float width = (float) m_MainTexture->GetWidth();
        float height = (float) m_MainTexture->GetHeight();

        int shadowEnabled = renderSettings.ShadowsEnabled ? 1 : 0;
        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "ViewMatrix", &view);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "LightView", &LightView);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "ShadowTransform", shadowTransforms);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "SplitDepths", uSplitDepth);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "BiasMatrix", &m_ForwardData.m_BiasMatrix);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "LightSize", &LightSize);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "ShadowFade", &ShadowFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "CascadeFade", &transitionFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "MaxShadowDist", &MaxShadowDist);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "InitialBias", &bias);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "Width", &width);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "Height", &height);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "shadowEnabled", &shadowEnabled);

            m_ForwardData.m_DescriptorSet[2]->SetTexture("uShadowMap",
                                                         reinterpret_cast<Texture*>(shadowData.m_ShadowTex), 0,
                                                         TextureType::DEPTHARRAY);

            int numShadows = shadowData.m_ShadowMapNum;
            auto EnvMipCount = m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap->GetMipMapLevels() : 0;
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "LightCount", &numLights);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "ShadowCount", &numShadows);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "Mode", &m_ForwardData.m_RenderMode);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UBOLight", "EnvMipCount", &EnvMipCount);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uBRDFLUT", m_ForwardData.m_BRDFLUT.get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uSSAOMap",
                                                         gEngine->GetLevelManager()->GetCurrentLevel()->GetSettings().renderSettings.SSAOEnabled
                                                         ? m_SSAOTexture : Material::GetDefaultTexture().get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uEnvMap", m_ForwardData.m_EnvironmentMap, 0,
                                                         TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uIrrMap", m_ForwardData.m_IrradianceMap, 0,
                                                         TextureType::CUBE);

            auto group = registry.group<ModelComponent>(entt::get<Transform>);

            PipelineDesc pipelineDesc = {};
            pipelineDesc.shader = m_ForwardData.m_Shader;
            pipelineDesc.polygonMode = PolygonMode::FILL;
            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.isClearTargets = false;
            pipelineDesc.isSwapChainTarget = false;

            for(auto entity: group)
            {
                if(!Entity(entity, level).Active())
                    continue;

                const auto &[model, trans] = group.get<ModelComponent, Transform>(entity);

                if(!model.model)
                    continue;

                const auto &meshes = model.model->GetMeshes();

                for(auto mesh: meshes)
                {
                    if(!mesh->IsActive())
                        continue;

                    auto &worldTransform = trans.GetWorldMatrix();
                    auto bbCopy = mesh->GetBoundingBox()->Transformed(worldTransform);

                    if(directionaLight)
                    {
                        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                        {
                            auto inside = m_ShadowData.m_CascadeFrustums[i].IsInside(bbCopy);

                            if(!inside)
                                continue;

                            RenderCommand command;
                            command.mesh = mesh.get();
                            command.transform = worldTransform;
                            command.material = mesh->GetMaterial() ? mesh->GetMaterial().get()
                                                                   : m_ForwardData.m_DefaultMaterial;

                            // Bind here in case not bound in the loop below as meshes will be inside
                            // cascade frustum and not the cameras
                            command.material->Bind();

                            m_ShadowData.m_CascadeCommandQueue[i].push_back(command);
                        }
                    }

                    {
                        auto inside = m_ForwardData.m_Frustum.IsInside(bbCopy);

                        if(!inside)
                            continue;

                        RenderCommand command;
                        command.mesh = mesh.get();
                        command.transform = worldTransform;
                        command.material = mesh->GetMaterial() ? mesh->GetMaterial().get()
                                                               : m_ForwardData.m_DefaultMaterial;

                        // Update material buffers
                        command.material->Bind();

                        pipelineDesc.colourTargets[0] = m_MainTexture;
                        pipelineDesc.cullMode = command.material->GetFlag(Material::RenderFlags::TWOSIDED)
                                                ? CullMode::NONE : CullMode::BACK;
                        pipelineDesc.isTransparencyEnabled = command.material->GetFlag(
                                Material::RenderFlags::ALPHABLEND);

                        if(m_ForwardData.m_DepthTest && command.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                        {
                            pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
                        }

//                        pipelineDesc.DebugName = fmt::format("Forward PBR {0} {1}",
//                                                             pipelineDesc.isTransparencyEnabled ? "Transparent" : "",
//                                                             pipelineDesc.depthTarget ? "DepthTested" : "");

                        command.pipeline = Pipeline::Get(pipelineDesc).get();

                        m_ForwardData.m_CommandQueue.push_back(command);
                    }
                }
            }
        }

        m_Renderer2DData.m_CommandQueue2D.clear();

//        if(renderSettings.Renderer2DEnabled)
//        {
//            auto spriteGroup = registry.group<Sprite>(entt::get<Transform>);
//            for(auto entity : spriteGroup)
//            {
//                const auto& [sprite, trans] = spriteGroup.get<Sprite, Transform>(entity);
//
//                auto bb = BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
//                bb.Transform(trans.GetWorldMatrix());
//                auto inside = m_ForwardData.m_Frustum.IsInside(bb);
//
//                if(!inside)
//                    continue;
//
//                RenderCommand2D command;
//                command.renderable = &sprite;
//                command.transform  = trans.GetWorldMatrix();
//                m_Renderer2DData.m_CommandQueue2D.push_back(command);
//            };
//
//            auto animSpriteGroup = registry.group<AnimatedSprite>(entt::get<Transform>);
//            for(auto entity : animSpriteGroup)
//            {
//                const auto& [sprite, trans] = animSpriteGroup.get<AnimatedSprite, Transform>(entity);
//
//                auto bb = BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
//                bb.Transform(trans.GetWorldMatrix());
//                auto inside = m_ForwardData.m_Frustum.IsInside(bb);
//
//                if(!inside)
//                    continue;
//
//                RenderCommand2D command;
//                command.renderable = &sprite;
//                command.transform  = trans.GetWorldMatrix();
//                m_Renderer2DData.m_CommandQueue2D.push_back(command);
//            };
//
//            {
//                NekoEngine_PROFILE_SCOPE("Sort Meshes by distance from camera");
//                auto camTransform = m_CameraTransform;
//                std::sort(m_ForwardData.m_CommandQueue.begin(), m_ForwardData.m_CommandQueue.end(),
//                          [camTransform](RenderCommand& a, RenderCommand& b)
//                          {
//                              if(a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && !b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
//                                  return true;
//                              if(!a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
//                                  return false;
//
//                              return glm::length(camTransform->GetWorldPosition() - glm::vec3(a.transform[3])) < glm::length(camTransform->GetWorldPosition() - glm::vec3(b.transform[3]));
//                          });
//            }
//
//            {
//                NekoEngine_PROFILE_SCOPE("Sort sprites by z value");
//                std::sort(m_Renderer2DData.m_CommandQueue2D.begin(), m_Renderer2DData.m_CommandQueue2D.end(),
//                          [](RenderCommand2D& a, RenderCommand2D& b)
//                          {
//                              return a.transform[3].z < b.transform[3].z;
//                          });
//            }
//        }
    }

    void SceneRenderer::SetRenderTarget(Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {

        m_ForwardData.m_RenderTexture = texture;
    }

    void SceneRenderer::OnRender()
    {

        auto &LevelRenderSettings = gEngine->GetLevelManager()->GetCurrentLevel()->GetSettings().renderSettings;
        gEngine->GetRenderer()->ClearRenderTarget(m_MainTexture, GET_SWAP_CHAIN()->GetCurrentCommandBuffer());

        LevelRenderSettings.SSAOEnabled = false;
        if(LevelRenderSettings.SSAOEnabled)
            gEngine->GetRenderer()->ClearRenderTarget(m_NormalTexture, GET_SWAP_CHAIN()->GetCurrentCommandBuffer());

        // Set to default texture if bloom disabled
        m_BloomTextureLastRenderered = Material::GetDefaultTexture().get();

        if(m_ForwardData.m_DepthTest)
        {
            gEngine->GetRenderer()->ClearRenderTarget(dynamic_cast<Texture*>(m_ForwardData.m_DepthTexture), GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
        }

        GenerateBRDFLUTPass();
        DepthPrePass();

        if(LevelRenderSettings.SSAOEnabled && !m_DisablePostProcess)
        {
            SSAOPass();

            if(LevelRenderSettings.SSAOBlur)
                SSAOBlurPass();
        }

        if(m_Settings.ShadowPass && LevelRenderSettings.ShadowsEnabled)
            ShadowPass();
        if(m_Settings.GeomPass && LevelRenderSettings.Renderer3DEnabled)
            ForwardPass();
        if(m_Settings.SkyboxPass && LevelRenderSettings.SkyboxRenderEnabled)
            SkyboxPass();
        if(m_Settings.GeomPass && LevelRenderSettings.Renderer2DEnabled)
            Render2DPass();

        TextPass();

        m_LastRenderTarget = m_MainTexture;

        // if (LevelRenderSettings.EyeAdaptation)
        //    EyeAdaptationPass();

        if(LevelRenderSettings.BloomEnabled && !m_DisablePostProcess)
            BloomPass();

        if(LevelRenderSettings.DepthOfFieldEnabled && !m_DisablePostProcess)
            DepthOfFieldPass();

        if(LevelRenderSettings.DebandingEnabled && !m_DisablePostProcess)
            DebandingPass();

        ToneMappingPass();

        if(LevelRenderSettings.SharpenEnabled && !m_DisablePostProcess)
            SharpenPass();

        if(LevelRenderSettings.FXAAEnabled && !m_DisablePostProcess)
            FXAAPass();

        if(LevelRenderSettings.ChromaticAberationEnabled && !m_DisablePostProcess)
            ChromaticAberationPass();

        if(LevelRenderSettings.FilmicGrainEnabled && !m_DisablePostProcess)
            FilmicGrainPass();

        // if(LevelRenderSettings.OutlineEnabled
        // OutlinePass();

//        if(m_Settings.DebugPass && LevelRenderSettings.DebugRenderEnabled)
//            DebugPass();

        FinalPass();
    }

    void SceneRenderer::OnUpdate(const TimeStep &timeStep, Level* level)
    {
    }


    std::string RenderModeToString(int mode)
    {
        switch(mode)
        {
            case 0:
                return "Lighting";
            case 1:
                return "Colour";
            case 2:
                return "Metallic";
            case 3:
                return "Roughness";
            case 4:
                return "AO";
            case 5:
                return "Emissive";
            case 6:
                return "Normal";
            case 7:
                return "Shadow Cascades";
            default:
                return "Lighting";
        }
    }

    void SceneRenderer::OnImGui()
    {


        ImGui::TextUnformatted("Shadow Renderer");

        if(ImGui::TreeNode("Texture"))
        {
            static int index = 0;

            // TODO: Fix - only showing layer 0
            ImGui::SliderInt("Texture Array Index", &index, 0, m_ShadowData.m_ShadowTex->GetCount());

            bool flipImage = gEngine->GetRenderer()->GetGraphicsContext()->FlipImGUITexture();

            ImGui::Image(m_ShadowData.m_ShadowTex->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f),
                         ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Image(m_ShadowData.m_ShadowTex->GetHandle(), ImVec2(256, 256),
                             ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }

        ImGui::DragFloat("Initial Bias", &m_ShadowData.m_InitialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
        ImGui::DragFloat("Light Size", &m_ShadowData.m_LightSize, 0.00005f, 0.0f, 10.0f);
        ImGui::DragFloat("Max Shadow Distance", &m_ShadowData.m_MaxShadowDistance, 0.05f, 0.0f, 10000.0f);
        ImGui::DragFloat("Shadow Fade", &m_ShadowData.m_ShadowFade, 0.0005f, 0.0f, 500.0f);
        ImGui::DragFloat("Cascade Transition Fade", &m_ShadowData.m_CascadeFade, 0.0005f, 0.0f, 5.0f);

        ImGui::DragFloat("Cascade Split Lambda", &m_ShadowData.m_CascadeSplitLambda, 0.005f, 0.0f, 3.0f);

        ImGui::TextUnformatted("Forward Renderer");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Renderables");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2lu", m_ForwardData.m_CommandQueue.size());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Render Mode");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(RenderModeToString(m_ForwardData.m_RenderMode).c_str()))
        {
            const int numRenderModes = 8;

            for(int i = 0; i < numRenderModes; i++)
            {
                if(ImGui::MenuItem(RenderModeToString(i).c_str(), "", m_ForwardData.m_RenderMode == i, true))
                {
                    m_ForwardData.m_RenderMode = i;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::TextUnformatted("2D renderer");
        ImGui::Columns(2);

        ImGuiUtility::Property("Number of draw calls", (int &) m_Renderer2DData.m_BatchDrawCallIndex,
                                 ImGuiUtility::PropertyFlag::ReadOnly);
        ImGuiUtility::Property("Max textures Per draw call", (int &) m_Renderer2DData.m_Limits.MaxTextures, 1, 16);
        ImGuiUtility::Property("Exposure", m_Exposure);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void SceneRenderer::OnNewLevel(Level* level)
    {
        m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
        m_ForwardData.m_IrradianceMap = m_DefaultTextureCube;

        DescriptorDesc info = {};
        info.shader = m_ForwardData.m_Shader.get();
        info.layoutIndex = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(info));
        m_CubeMap = nullptr;

        DescriptorDesc descriptorDesc = {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_SkyboxShader.get();
        m_SkyboxDescriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
    }

    float RoundUpToNearestMultipleOf5(float value)
    {
        // Divide the value by 5 and round it up to the nearest integer
        int roundedValue = static_cast<int>(std::ceil(value / 5.0));

        // Multiply the rounded value by 5 to get the nearest multiple of 5
        float result = roundedValue * 5.0f;

        return result;
    }

    void SceneRenderer::UpdateCascades(Level* level, Light* light)
    {

        float cascadeSplits[SHADOWMAP_MAX];

        float cascadeRadius[SHADOWMAP_MAX];

        float nearClip = m_Camera->GetNear();
        float farClip = m_Camera->GetFar();
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;
        float range = maxZ - minZ;
        float ratio = maxZ / minZ;
        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            float p = static_cast<float>(i + 1) / static_cast<float>(m_ShadowData.m_ShadowMapNum);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = m_ShadowData.m_CascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        cascadeSplits[3] = 0.35f;
        float lastSplitDist = 0.0f;

        const glm::mat4 invCam = glm::inverse(
                m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix()));

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                    glm::vec3(-1.0f, 1.0f, -1.0f),
                    glm::vec3(1.0f, 1.0f, -1.0f),
                    glm::vec3(1.0f, -1.0f, -1.0f),
                    glm::vec3(-1.0f, -1.0f, -1.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            // Project frustum corners into world space
            for(uint32_t j = 0; j < 8; j++)
            {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
                frustumCorners[j] = (invCorner / invCorner.w);
            }

            for(uint32_t j = 0; j < 4; j++)
            {
                glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }

            lastSplitDist = cascadeSplits[i];

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for(uint32_t j = 0; j < 8; j++)
            {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for(uint32_t j = 0; j < 8; j++)
            {
                float distance = glm::distance(frustumCorners[j], frustumCenter);
                radius = Maths::Max(radius, distance);
            }

            // Temp work around to flickering when rotating camera
            // Sphere radius for lightOrthoMatrix should fix this
            // But radius changes as the camera is rotated which causes flickering
            // const float value = 16.0f;
            // radius = std::ceil(radius *value) / value;

            static const float roundTo[8] = {5.0f, 5.0f, 20.0f, 200.0f, 400.0f, 400.0f, 400.0f, 400.0f};
            radius = RoundUpToNearestMultipleOf5(radius);

            cascadeRadius[i] = radius;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = glm::normalize(-light->Direction);
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y,
                                                    m_ShadowData.CascadeNearPlaneOffset,
                                                    maxExtents.z - minExtents.z + m_ShadowData.CascadeFarPlaneOffset);
            glm::mat4 LightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter,
                                                    glm::vec3(0.0f, 0.0f, 1.0f));

            auto shadowProj = lightOrthoMatrix * LightViewMatrix;

            const bool StabilizeCascades = true;
            if(StabilizeCascades)
            {
                // Create the rounding matrix, by projecting the world-space origin and determining
                // the fractional offset in texel space
                glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                shadowOrigin = shadowProj * shadowOrigin;
                shadowOrigin *= (m_ShadowData.m_ShadowMapSize * 0.5f);

                glm::vec4 roundedOrigin = glm::round(shadowOrigin);
                glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
                roundOffset = roundOffset * (2.0f / m_ShadowData.m_ShadowMapSize);
                roundOffset.z = 0.0f;
                roundOffset.w = 0.0f;

                lightOrthoMatrix[3] += roundOffset;
            }
            // Store split distance and matrix in cascade
            m_ShadowData.m_SplitDepth[i] = glm::vec4((m_Camera->GetNear() + splitDist * clipRange) * -1.0f);
            m_ShadowData.m_ShadowProjView[i] = lightOrthoMatrix * LightViewMatrix;

            if(i == 0)
                m_ShadowData.m_LightMatrix = glm::inverse(LightViewMatrix);
        }
    }

    void SceneRenderer::GenerateBRDFLUTPass()
    {
        
        if(!m_GenerateBRDFLUT)
            return;

        m_GenerateBRDFLUT = false;

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = GET_SHADER_LIB()->GetResource("BRDFLUT");

        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.colourTargets[0] = m_ForwardData.m_BRDFLUT.get();
        pipelineDesc.DebugName = "BRDF Generation";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);

        pipeline->Bind(commandBuffer);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
        pipeline->End(commandBuffer);
    }

    void SceneRenderer::ShadowPass()
    {
        bool empty = true;
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            if(!m_ShadowData.m_CascadeCommandQueue[i].empty())
                empty = false;
        }

        if(empty)
            return;

        m_ShadowData.m_DescriptorSet[0]->SetUniform("ShadowData", "LightMatrices", m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[0]->Update();

        PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_ShadowData.m_Shader;
        pipelineDesc.cullMode = CullMode::FRONT;
        pipelineDesc.isTransparencyEnabled = true; // For alpha cutout
        pipelineDesc.depthArrayTarget = reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex);
        pipelineDesc.isClearTargets = true;
        pipelineDesc.isDepthBiasEnabled = false;
        pipelineDesc.depthBiasConstantFactor = 0.0f;
        pipelineDesc.depthBiasSlopeFactor = 0.0f;
        pipelineDesc.DebugName = "Shadow";

        auto pipeline = Pipeline::Get(pipelineDesc);
        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            m_ShadowData.m_Layer = i;
            pipeline->Bind(commandBuffer, m_ShadowData.m_Layer);

            uint32_t layer = static_cast<uint32_t>(m_ShadowData.m_Layer);
            auto &pushConstants = m_ShadowData.m_Shader->GetPushConstants();
            memcpy(pushConstants[0].data + sizeof(glm::mat4), &layer, sizeof(uint32_t));
            m_ShadowData.m_CurrentDescriptorSets[0] = m_ShadowData.m_DescriptorSet[0].get();

            for(auto &command: m_ShadowData.m_CascadeCommandQueue[m_ShadowData.m_Layer])
            {
                m_Stats.NumShadowObjects++;

                Mesh* mesh = command.mesh;

                auto trans = command.transform;
                memcpy(pushConstants[0].data, &trans, sizeof(glm::mat4));

                Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                m_ShadowData.m_CurrentDescriptorSets[1] = material->GetDescriptorSet();

                m_ShadowData.m_Shader->BindPushConstants(commandBuffer, pipeline.get());
                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0,
                                                           m_ShadowData.m_CurrentDescriptorSets.data(), 2);
                gEngine->GetRenderer()->DrawMesh(commandBuffer, pipeline.get(), mesh);
            }

            pipeline->End(commandBuffer);
        }
    }

    void SceneRenderer::DepthPrePass()
    {
        
        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_DepthPrePassShader;
        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.isClearTargets = false;
        pipelineDesc.isSwapChainTarget = false;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
        pipelineDesc.colourTargets[0] = m_NormalTexture;
        pipelineDesc.DebugName = "Depth Prepass";

        auto pipeline = Pipeline::Get(pipelineDesc).get();
        commandBuffer->BindPipeline(pipeline);

        for(auto &command: m_ForwardData.m_CommandQueue)
        {
            if(!command.material->GetFlag(Material::RenderFlags::DEPTHTEST) ||
               command.material->GetFlag(Material::RenderFlags::ALPHABLEND))
                continue;

            Mesh* mesh = command.mesh;
            auto &worldTransform = command.transform;

            DescriptorSet* sets[2];
            sets[0] = m_ForwardData.m_DescriptorSet[0].get();

            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            sets[1] = material->GetDescriptorSet();

            auto &pushConstants = m_DepthPrePassShader->GetPushConstants()[0];
            pushConstants.SetValue("transform", (void*) &worldTransform);

            m_DepthPrePassShader->BindPushConstants(commandBuffer, pipeline);
            gEngine->GetRenderer()->BindDescriptorSets(pipeline, commandBuffer, 0, sets, 2);
            gEngine->GetRenderer()->DrawMesh(commandBuffer, pipeline, mesh);
        }

        if(commandBuffer)
            commandBuffer->UnBindPipeline();
    }

    void SceneRenderer::SSAOPass()
    {
        if(!m_Camera || !m_SSAOShader || !m_SSAOShader->IsCompiled())
            return;

        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_SSAOShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.DebugName = "SSAO";

        auto projection = m_Camera->GetProjectionMatrix();
        auto invProj = glm::inverse(m_Camera->GetProjectionMatrix());
        auto view = glm::inverse(m_CameraTransform->GetWorldMatrix());

        float nearC = m_Camera->GetNear();
        float farC = m_Camera->GetFar();

        static glm::vec4 samples[64];
        static bool init = false;

        if(!init)
        {
            for(uint32_t i = 0; i < 64; ++i)
            {
                glm::vec3 sample(Random32::Rand(-0.9f, 0.9f), Random32::Rand(-0.9f, 0.9f), Random32::Rand(0.0f, 1.0f));
                sample = glm::normalize(sample);      // Snap to surface of hemisphere
                sample *= Random32::Rand(0.0f, 1.0f); // Space out linearly
                float scale = (float) i / (float) 64;
                scale = Maths::Lerp(0.1f, 1.0f, scale * scale); // Bring distribution of samples closer to origin
                samples[i] = glm::vec4(sample * scale, 0.0f);
            }
            init = true;
        }

        LevelRenderSettings &renderSettings = m_CurrentScene->GetSettings().renderSettings;

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "invProj", &invProj);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "projection", &projection);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "view", &view);

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "samples", &samples);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "ssaoRadius", &renderSettings.SSAOSampleRadius);

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "near", &nearC);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "far", &farC);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "strength", &renderSettings.SSAOStrength);

        m_SSAOPassDescriptorSet->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOPassDescriptorSet->SetTexture("in_Noise", m_NoiseTexture);
        m_SSAOPassDescriptorSet->SetTexture("in_Normal", m_NormalTexture);
        m_SSAOPassDescriptorSet->Update();

        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_SSAOPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);
    }

    void SceneRenderer::SSAOBlurPass()
    {
        if(!m_Camera || !m_SSAOBlurShader || !m_SSAOBlurShader->IsCompiled())
            return;

        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_SSAOBlurShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture1;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.DebugName = "SSAO Blur";

        glm::vec2 ssaoTexelOffset = glm::vec2(0.0f, 2.0f / m_SSAOTexture->GetHeight());

        LevelRenderSettings &renderSettings = m_CurrentScene->GetSettings().renderSettings;

        m_SSAOBlurPassDescriptorSet->SetUniform("UniformBuffer", "ssaoTexelOffset", &ssaoTexelOffset);
        m_SSAOBlurPassDescriptorSet->SetUniform("UniformBuffer", "ssaoBlurRadius", &renderSettings.SSAOBlurRadius);

        m_SSAOBlurPassDescriptorSet->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture("in_SSAO", m_SSAOTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture("in_Normal", m_NormalTexture);
        m_SSAOBlurPassDescriptorSet->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet->Update();

        ssaoTexelOffset = glm::vec2(2.0f / m_SSAOTexture->GetWidth(), 0.0f);

        m_SSAOBlurPassDescriptorSet2->SetUniform("UniformBuffer", "ssaoTexelOffset", &ssaoTexelOffset);
        m_SSAOBlurPassDescriptorSet2->SetUniform("UniformBuffer", "ssaoBlurRadius", &renderSettings.SSAOBlurRadius);

        m_SSAOBlurPassDescriptorSet2->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet2->SetTexture("in_SSAO", m_SSAOTexture1);
        m_SSAOBlurPassDescriptorSet2->SetTexture("in_Normal", m_NormalTexture);

        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_SSAOBlurPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_SSAOBlurPassDescriptorSet2->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet2->Update();

        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        set = m_SSAOBlurPassDescriptorSet2.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);
    }

    void SceneRenderer::ForwardPass()
    {

        if(m_ForwardData.m_CommandQueue.empty())
            return;
        m_ForwardData.m_DescriptorSet[2]->Update();

        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        for(auto &command: m_ForwardData.m_CommandQueue)
        {
            m_Stats.NumRenderedObjects++;

            Mesh* mesh = command.mesh;
            auto &worldTransform = command.transform;
            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            auto pipeline = command.pipeline;
            commandBuffer->BindPipeline(pipeline);

            m_ForwardData.m_CurrentDescriptorSets[0] = m_ForwardData.m_DescriptorSet[0].get();
            m_ForwardData.m_CurrentDescriptorSets[1] = material->GetDescriptorSet();
            m_ForwardData.m_CurrentDescriptorSets[2] = m_ForwardData.m_DescriptorSet[2].get();

            auto &pushConstants = m_ForwardData.m_Shader->GetPushConstants()[0];
            pushConstants.SetValue("transform", (void*) &worldTransform);

            m_ForwardData.m_Shader->BindPushConstants(commandBuffer, pipeline);
            gEngine->GetRenderer()->BindDescriptorSets(pipeline, commandBuffer, 0,
                                                       m_ForwardData.m_CurrentDescriptorSets.data(), 3);
            gEngine->GetRenderer()->DrawMesh(commandBuffer, pipeline, mesh);
        }

        if(commandBuffer)
            commandBuffer->UnBindPipeline();
    }

    void SceneRenderer::SkyboxPass()
    {

        if(!m_CubeMap)
            return;

        if(!m_Camera || !m_CameraTransform)
            return;

        int mode = 0;
        float exposure = m_Exposure * 120000.0f;
        float blurLevel = m_CurrentScene->GetSettings().renderSettings.SkyboxMipLevel;
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "Mode", &mode);
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "Exposure", &exposure);
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "BlurLevel", &blurLevel);
        m_SkyboxDescriptorSet->SetTexture("u_CubeMap", m_CubeMap, 0, TextureType::CUBE);
        m_SkyboxDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_SkyboxShader;

        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;

        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        }

        pipelineDesc.colourTargets[0] = m_MainTexture;
        pipelineDesc.DebugName = "Skybox";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_SkyboxDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->DrawMesh(commandBuffer, pipeline.get(), m_ScreenQuad);

        pipeline->End(commandBuffer);
    }

    void SceneRenderer::DepthOfFieldPass()
    {
        if(!m_Camera || !m_DepthOfFieldShader || !m_DepthOfFieldShader->IsCompiled())
            return;

        const auto &proj = m_Camera->GetProjectionMatrix();
        LevelRenderSettings &renderSettings = m_CurrentScene->GetSettings().renderSettings;

        float depthLinearizeMul = (-proj[3][2]); // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
        float depthLinearizeAdd = (proj[2][2]);  // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
        // correct the handedness issue.
        if(depthLinearizeMul * depthLinearizeAdd < 0)
            depthLinearizeAdd = -depthLinearizeAdd;
        glm::vec2 DepthConsts = {depthLinearizeMul, depthLinearizeAdd};
        glm::vec2 DOFParams = {renderSettings.DepthOfFieldDistance, renderSettings.DepthOfFieldStrength};

        m_DepthOfFieldPassDescriptorSet->SetUniform("UniformBuffer", "DOFParams", &DOFParams);
        m_DepthOfFieldPassDescriptorSet->SetUniform("UniformBuffer", "DepthConsts", &DepthConsts);

        m_DepthOfFieldPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
        m_DepthOfFieldPassDescriptorSet->SetTexture("u_DepthTexture", m_ForwardData.m_DepthTexture);
        m_DepthOfFieldPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_DepthOfFieldShader;
        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "DepthofField";
        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_DepthOfFieldPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

//    void SceneRenderer::SharpenPass()
//    {
//        if(!m_Camera || !m_SharpenShader || !m_SharpenShader->IsCompiled())
//            return;
//
//        LevelRenderSettings& renderSettings = m_CurrentScene->GetSettings().renderSettings;
//
//        m_SharpenPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
//        m_SharpenPassDescriptorSet->Update();
//
//        PipelineDesc pipelineDesc {};
//        pipelineDesc.shader              = m_SharpenShader;
//        pipelineDesc.polygonMode         = PolygonMode::FILL;
//        pipelineDesc.cullMode            = CullMode::BACK;
//        pipelineDesc.isTransparencyEnabled = false;
//        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
//        pipelineDesc.DebugName           = "Sharpen";
//        auto commandBuffer               = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
//        auto pipeline                    = Pipeline::Get(pipelineDesc);
//        pipeline->Bind(commandBuffer);
//
//        auto set = m_SharpenPassDescriptorSet.get();
//        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
//        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
//
//        pipeline->End(commandBuffer);
//
//        m_LastRenderTarget = m_PostProcessTexture1;
//        std::swap(m_PostProcessTexture1, m_MainTexture);
//    }

    void SceneRenderer::ToneMappingPass()
    {
        float bloomIntensity = m_CurrentScene->GetSettings().renderSettings.BloomEnabled
                               ? m_CurrentScene->GetSettings().renderSettings.m_BloomIntensity : 0.0f;
        float Saturation = m_CurrentScene->GetSettings().renderSettings.Saturation;
        float Brightness = m_CurrentScene->GetSettings().renderSettings.Brightness;
        float Contrast = m_CurrentScene->GetSettings().renderSettings.Contrast;

        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "BloomIntensity", &bloomIntensity);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "ToneMapIndex", &m_ToneMapIndex);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Brightness", &Brightness);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Contrast", &Contrast);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Saturation", &Saturation);

        m_ToneMappingPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
        m_ToneMappingPassDescriptorSet->SetTexture("u_BloomTexture", m_BloomTextureLastRenderered);
        m_ToneMappingPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_ToneMappingPassShader;
        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "ToneMapping";
        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_ToneMappingPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::FinalPass()
    {
        Texture* finalPassTexture = m_MainTexture;

#ifndef NekoEngine_DIST

        int debugMode = m_CurrentScene->GetSettings().renderSettings.DebugMode;
        switch(debugMode)
        {
            case 1:
                finalPassTexture = m_SSAOTexture;
                break;
            case 2:
                finalPassTexture = m_SSAOTexture1;
                break;
            case 3:
                finalPassTexture = m_NormalTexture;
                break;
            case 4:
                finalPassTexture = m_BloomTexture;
                break;
            case 5:
                finalPassTexture = m_NoiseTexture;
                break;
            case 6:
                finalPassTexture = m_PostProcessTexture1;
                break;
            case 0:
            default:
                break;
        }
#endif
        m_FinalPassDescriptorSet->SetTexture("u_Texture", finalPassTexture);
        m_FinalPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_FinalPassShader;

        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.DebugName = "Final Pass";
        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.isSwapChainTarget = true;

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_FinalPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);
    }

    void SceneRenderer::BloomPass()
    {

        if(!m_BloomPassShader->IsCompiled() || !m_BloomDescriptorSets[0] || m_BloomTexture->GetWidth() == 0 ||
           m_BloomTexture->GetHeight() == 0)
            return;

        int descCount = (int) m_BloomDescriptorSets.size();
        int targetMipCount = m_BloomTexture->GetMipMapLevels();

        int targetDescCount = 1 + (targetMipCount - 2) * 2 + 1 + (targetMipCount - 2 - 3);

        if(descCount != targetDescCount)
        {
            m_BloomDescriptorSets.clear();

            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = m_BloomPassShader.get();

            for(int i = 0; i < targetDescCount; i++)
            {
                m_BloomDescriptorSets.push_back(
                        SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc)));
            }
        }

        int passCount = 0;

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        struct BloomComputePushConstants
        {
            glm::vec4 Params;
            glm::vec4 Params2; // float LOD = 0.0f;
            // int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
        } bloomComputePushConstants;

        bloomComputePushConstants.Params = {m_CurrentScene->GetSettings().renderSettings.BloomThreshold,
                                            m_CurrentScene->GetSettings().renderSettings.BloomThreshold -
                                            m_CurrentScene->GetSettings().renderSettings.BloomKnee,
                                            m_CurrentScene->GetSettings().renderSettings.BloomKnee * 2.0f,
                                            0.25f / m_CurrentScene->GetSettings().renderSettings.BloomKnee};
        bloomComputePushConstants.Params2.x = (float) 0;
        bloomComputePushConstants.Params2.y = (float) 0;
        bloomComputePushConstants.Params2.z = (float) m_MainTexture->GetWidth();
        bloomComputePushConstants.Params2.w = (float) m_MainTexture->GetHeight();

        uint32_t workGroupSize = 4;
        uint32_t workGroupsX = m_BloomTexture->GetWidth() / workGroupSize;
        uint32_t workGroupsY = m_BloomTexture->GetHeight() / workGroupSize;

        m_BloomDescriptorSets[0]->SetTexture("u_Texture", m_MainTexture);
        m_BloomDescriptorSets[0]->SetTexture("u_BloomTexture", m_MainTexture);

        if(m_SupportCompute)
        {
            m_BloomDescriptorSets[0]->SetTexture("o_Image", m_BloomTexture);
            m_BloomDescriptorSets[0]->TransitionImages(commandBuffer);
        }
        m_BloomDescriptorSets[0]->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_BloomPassShader;
        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.mipIndex = 0;
        pipelineDesc.DebugName = "Bloom-Prefilter";
        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_BloomTexture;

        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto &pushConstants = m_BloomPassShader->GetPushConstants();
        memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
        m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

        // Pre Filter
        {

            auto set = m_BloomDescriptorSets[passCount].get();
            gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

            if(m_SupportCompute)
            {
                gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;
            pipeline->End(commandBuffer);
        }

        bloomComputePushConstants.Params2.y = 1;
        uint32_t mips;

        if(m_BloomTexture->GetMipMapLevels() < 2)
            mips = 1;
        else
            mips = m_BloomTexture->GetMipMapLevels() - 2;

        for(int i = 1; i < (targetMipCount - 2) * 2; i++)
        {
            if(i % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture1);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_MainTexture);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_MainTexture);
            }

            m_BloomDescriptorSets[i]->Update();
        }

        //        m_BloomDescriptorSets[1]->SetTexture("u_Texture", m_BloomTexture);
        //        m_BloomDescriptorSets[1]->SetTexture("u_BloomTexture", m_MainTexture);
        //
        //        m_BloomDescriptorSets[2]->SetTexture("u_Texture", m_BloomTexture1);
        //        m_BloomDescriptorSets[2]->SetTexture("u_BloomTexture", m_MainTexture);

        // m_BloomDescriptorSets[2]->Update();

        for(uint32_t i = 1; i < mips; i++)
        {
            {
                pipelineDesc.mipIndex = i;
//                pipelineDesc.DebugName = fmt::format("Bloom-Downsample{0}", i);
                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture1;
                bloomComputePushConstants.Params2.z = (float) m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float) m_BloomTexture1->GetHeight(i);

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture1, i);
                }

                m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                m_BloomDescriptorSets[passCount]->Update();

                pipeline = Pipeline::Get(pipelineDesc);
                pipeline->Bind(commandBuffer);

                bloomComputePushConstants.Params2.x = i - 1.0f;

                auto &pushConstants = m_BloomPassShader->GetPushConstants();
                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t) glm::ceil((float) m_BloomTexture->GetWidth(i) / (float) workGroupSize);
                    workGroupsY = (uint32_t) glm::ceil((float) m_BloomTexture->GetHeight(i) / (float) workGroupSize);

                    gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                }
                else
                {
                    gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;

                pipeline->End(commandBuffer);
            }
            {

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                {
                    // m_BloomDescriptorSets[passCount]->SetTexture("u_Texture", m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }

                pipelineDesc.mipIndex = i;

                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture;
                bloomComputePushConstants.Params2.z = (float) m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float) m_BloomTexture1->GetHeight(i);

                pipeline = Pipeline::Get(pipelineDesc);
                pipeline->Bind(commandBuffer);

                bloomComputePushConstants.Params2.x = (float) i;

                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t) glm::ceil((float) m_BloomTexture->GetWidth(i) / (float) workGroupSize);
                    workGroupsY = (uint32_t) glm::ceil((float) m_BloomTexture->GetHeight(i) / (float) workGroupSize);

                    gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                }
                else
                {
                    gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;

                pipeline->End(commandBuffer);
            }
        }

        // First Upsample
        {
            m_BloomDescriptorSets[passCount]->SetTexture("u_Texture", m_BloomTexture);
            m_BloomDescriptorSets[passCount]->SetTexture("u_BloomTexture", m_MainTexture);

            if(m_SupportCompute)
            {
                m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture2, mips - 2);
            }
            m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            m_BloomDescriptorSets[passCount]->Update();

            bloomComputePushConstants.Params2.y = (float) 2;
            bloomComputePushConstants.Params2.x -= 1.0f;
            bloomComputePushConstants.Params2.z = (float) m_BloomTexture2->GetWidth(mips - 2);
            bloomComputePushConstants.Params2.w = (float) m_BloomTexture2->GetHeight(mips - 2);

            pipelineDesc.mipIndex = mips - 2;
//            pipelineDesc.DebugName = fmt::format("Bloom-Upsample{0}", mips - 2);

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = m_BloomTexture2;
            pipeline = Pipeline::Get(pipelineDesc);
            pipeline->Bind(commandBuffer);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            auto set2 = m_BloomDescriptorSets[passCount].get();
            gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t) glm::ceil((float) m_BloomTexture->GetWidth(mips - 2) / (float) workGroupSize);
                workGroupsY = (uint32_t) glm::ceil((float) m_BloomTexture->GetHeight(mips - 2) / (float) workGroupSize);

                gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;

            pipeline->End(commandBuffer);
        }

        int test = 1;
        for(int i = passCount; i < passCount + (targetMipCount - 2 - 3) + 1; i++)
        {
            if(test % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_BloomTexture1);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_BloomTexture2);
            }
            m_BloomDescriptorSets[i]->Update();

            test++;
        }

        // Can't transition images to right layout if render to a colour attachment
        // Need to sample from m_BloomTexture2 first then switch between that and m_BloomTexture1
        bool evenMip = true;

        // Upsample
        for(int32_t mip = mips - 3; mip >= 0; mip--)
        {
            bloomComputePushConstants.Params2.y = (float) 3;
            bloomComputePushConstants.Params2.x = (float) mip;
            bloomComputePushConstants.Params2.z = (float) m_BloomTexture2->GetWidth(mip);
            bloomComputePushConstants.Params2.w = (float) m_BloomTexture2->GetHeight(mip);

            pipelineDesc.mipIndex = mip;
//            pipelineDesc.DebugName = fmt::format("Bloom-Upsample{0}", mip);

            if(evenMip)
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture1, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }
            else
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture2, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            auto set2 = m_BloomDescriptorSets[passCount].get(); // evenMip ? m_BloomDescriptorSets[passCount].get() : m_BloomDescriptorSets[5].get();
            set2->TransitionImages(commandBuffer);

            pipeline = Pipeline::Get(pipelineDesc);
            pipeline->Bind(commandBuffer);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t) glm::ceil((float) m_BloomTexture2->GetWidth(mip) / (float) workGroupSize);
                workGroupsY = (uint32_t) glm::ceil((float) m_BloomTexture2->GetHeight(mip) / (float) workGroupSize);

                gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;

            pipeline->End(commandBuffer);

            m_BloomTextureLastRenderered = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            evenMip = !evenMip;
        }
    }

    void SceneRenderer::FXAAPass()
    {
        if(!m_MainTexture || !m_FXAAShader->IsCompiled())
            return;

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        m_FXAAPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);

        if(m_SupportCompute)
        {
            m_FXAAPassDescriptorSet->SetTexture("o_Image", m_PostProcessTexture1);
            m_FXAAPassDescriptorSet->TransitionImages(commandBuffer);
        }

        m_FXAAPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_FXAAShader;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.isClearTargets = false;
        pipelineDesc.DebugName = "FXAA";

        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_PostProcessTexture1;

        auto pipeline = Pipeline::Get(pipelineDesc);

        pipeline->Bind(commandBuffer);

        auto set = m_FXAAPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

        if(m_SupportCompute)
        {
            uint32_t workGroupSize = 4;
            uint32_t workGroupsX = m_PostProcessTexture1->GetWidth() / workGroupSize;
            uint32_t workGroupsY = m_PostProcessTexture1->GetHeight() / workGroupSize;

            gEngine->GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
        }
        else
        {
            gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);
        }

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::DebandingPass()
    {
        if(!m_MainTexture || !m_DebandingShader->IsCompiled())
            return;

        m_DebandingPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
        m_DebandingPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_DebandingShader;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "Debanding";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto set = m_DebandingPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::FilmicGrainPass()
    {
        if(!m_MainTexture || !m_FilmicGrainShader->IsCompiled())
            return;

        m_FilmicGrainPassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
        m_FilmicGrainPassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_FilmicGrainShader;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "FilmicGrain";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        auto &pushConstants = m_FilmicGrainShader->GetPushConstants();
        if(!pushConstants.empty())
        {
            auto &pushConstant = m_FilmicGrainShader->GetPushConstants()[0];
            float time = (float) gEngine->GetTimeStep()->GetElapsedSeconds();
            float intensity = 0.02f;
            pushConstant.SetValue("Time", (void*) &time);
            pushConstant.SetValue("Intensity", (void*) &intensity);
            m_FilmicGrainShader->BindPushConstants(commandBuffer, pipeline.get());
        }

        auto set = m_FilmicGrainPassDescriptorSet.get();
        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::OutlinePass()
    {
        if(!m_MainTexture || !m_OutlineShader || !m_OutlineShader->IsCompiled())
            return;

        // m_OutlinePassDescriptorSet->SetTexture("u_Texture", m_MainTexture);
        // m_OutlinePassDescriptorSet->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_OutlineShader;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "Outline";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        // auto set = m_OutlinePassDescriptorSet.get();
        // gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::ChromaticAberationPass()
    {
        if(!m_Camera || !m_MainTexture || !m_ChromaticAberationShader->IsCompiled())
            return;

        auto set = m_ChromaticAberationPassDescriptorSet.get();
        float cameraAperture = m_Camera->GetAperture();
        float intensity = 100.0f;

        set->SetTexture("u_Texture", m_MainTexture);
        set->SetUniform("UniformBuffer", "chromaticAberrationIntensity", &intensity);
        set->SetUniform("UniformBuffer", "cameraAperture", &cameraAperture);
        set->Update();

        PipelineDesc pipelineDesc{};
        pipelineDesc.shader = m_ChromaticAberationShader;
        pipelineDesc.isTransparencyEnabled = false;
        pipelineDesc.isClearTargets = true;
        pipelineDesc.colourTargets[0] = m_PostProcessTexture1;
        pipelineDesc.DebugName = "ChromaticAberation";

        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
        auto pipeline = Pipeline::Get(pipelineDesc);
        pipeline->Bind(commandBuffer);

        gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

        pipeline->End(commandBuffer);

        m_LastRenderTarget = m_PostProcessTexture1;
        std::swap(m_PostProcessTexture1, m_MainTexture);
    }

    void SceneRenderer::EyeAdaptationPass()
    {
    }

    float SceneRenderer::SubmitTexture(Texture* texture)
    {

        float result = 0.0f;
        bool found = false;
        for(uint32_t i = 0; i < m_Renderer2DData.m_TextureCount; i++)
        {
            if(m_Renderer2DData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found = true;
                break;
            }
        }

        if(!found)
        {
            if(m_Renderer2DData.m_TextureCount >= m_Renderer2DData.m_Limits.MaxTextures)
            {
                Render2DFlush();
            }
            m_Renderer2DData.m_Textures[m_Renderer2DData.m_TextureCount] = texture;
            m_Renderer2DData.m_TextureCount++;
            result = static_cast<float>(m_Renderer2DData.m_TextureCount);
        }
        return result;
    }

    void SceneRenderer::Render2DPass()
    {
        if(m_Renderer2DData.m_CommandQueue2D.empty())
            return;

        PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_Renderer2DData.m_Shader;
        pipelineDesc.polygonMode = PolygonMode::FILL;
        pipelineDesc.cullMode = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = true;
        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.isClearTargets = false;
        pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.colourTargets[0] = m_MainTexture;
        pipelineDesc.DebugName = "2D";

        m_Renderer2DData.m_Pipeline = Pipeline::Get(pipelineDesc);

        uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();

        Renderer2DBeginBatch();

        auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
        m_Renderer2DData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
        m_Renderer2DData.m_DescriptorSet[0][0]->Update();

        for(auto &command: m_Renderer2DData.m_CommandQueue2D)
        {
            m_Stats.NumRenderedObjects++;

            if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
                Render2DFlush();

            auto &renderable = command.renderable;
            auto &transform = command.transform;

            const glm::vec2 min = renderable->GetPosition();
            const glm::vec2 max = renderable->GetPosition() + renderable->GetScale();

            const glm::vec4 colour = renderable->GetColour();
            const auto &uv = renderable->GetUVs();
            Texture* texture = renderable->GetTexture().get();

            float textureSlot = 0.0f;
            if(texture)
                textureSlot = SubmitTexture(texture);

            glm::vec3 vertex = transform * glm::vec4(min.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[0];
            m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * glm::vec4(max.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[1];
            m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * glm::vec4(max.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[2];
            m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * glm::vec4(min.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[3];
            m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            m_Renderer2DData.m_IndexCount += 6;
        }

        if(m_Renderer2DData.m_IndexCount == 0)
        {
            // m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();
            return;
        }

        Render2DFlush();

        // m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();
    }

    void SceneRenderer::Renderer2DBeginBatch()
    {
        uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();

        m_Renderer2DData.m_IndexCount = 0;
        m_Renderer2DData.m_TextureCount = 0;

        if((int) m_Renderer2DData.m_VertexBuffers[currentFrame].size() - 1 <
           (int) m_Renderer2DData.m_BatchDrawCallIndex)
        {
            auto &vertexBuffer = m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(m_Renderer2DData.m_Limits.BufferSize);

            m_2DBufferBase[currentFrame].emplace_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads * 4]);
        }

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Bind(
                GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_Renderer2DData.m_Pipeline.get());
        m_Renderer2DData.m_Buffer = m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex];
    }

    void SceneRenderer::Render2DFlush()
    {

        uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();

        if(m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] == nullptr ||
           m_Renderer2DData.m_TextureCount !=
           m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 1;
            descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] = SharedPtr<DescriptorSet>(
                    GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        if(m_Renderer2DData.m_TextureCount > 1)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures",
                                                                                                   m_Renderer2DData.m_Textures,
                                                                                                   m_Renderer2DData.m_TextureCount);
        else if(m_Renderer2DData.m_TextureCount == 0)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures", nullptr);
        else
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures",
                                                                                                   m_Renderer2DData.m_Textures[0]);

        m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->Update();

        m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex] = m_Renderer2DData.m_TextureCount;

        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        // m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();

        uint32_t dataSize = (uint32_t) ((uint8_t*) m_Renderer2DData.m_Buffer -
                                        (uint8_t*) m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]);
        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->SetData(dataSize,
                                                                                                       (void*) m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]);

        m_Renderer2DData.m_Pipeline->Bind(commandBuffer);

        m_Renderer2DData.m_CurrentDescriptorSets[0] = m_Renderer2DData.m_DescriptorSet[0][0].get();
        m_Renderer2DData.m_CurrentDescriptorSets[1] = m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1].get();

        m_Renderer2DData.m_IndexBuffer->SetCount(m_Renderer2DData.m_IndexCount);
        m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

        gEngine->GetRenderer()->BindDescriptorSets(m_Renderer2DData.m_Pipeline.get(), commandBuffer, 0,
                                                   m_Renderer2DData.m_CurrentDescriptorSets.data(), 2);
        gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_Renderer2DData.m_IndexCount);

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
        m_Renderer2DData.m_IndexBuffer->Unbind();

        m_Renderer2DData.m_Pipeline->End(commandBuffer);

        m_Renderer2DData.m_BatchDrawCallIndex++;
        Renderer2DBeginBatch();

        /*m_Renderer2DData.m_IndexCount = 0;
        m_Renderer2DData.m_TextureCount = 0;

        if((int)m_Renderer2DData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_Renderer2DData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(m_Renderer2DData.m_Limits.BufferSize);

            m_2DBufferBase[currentFrame].emplace_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads  * 4]);
        }

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Bind(GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_Renderer2DData.m_Pipeline.get());
        m_Renderer2DData.m_Buffer = m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex];*/
        // m_Renderer2DData.m_Buffer = m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();
    }

    void SceneRenderer::TextFlush(Renderer2DData &textRenderData, std::vector<TextVertexData*> &textVertexBufferBase,
                                  TextVertexData*&textVertexBufferPtr)
    {

        uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();

        if(textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] == nullptr)
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 1;
            descriptorDesc.shader = textRenderData.m_Shader.get();
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] = SharedPtr<DescriptorSet>(
                    GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        }

        if(textRenderData.m_TextureCount > 1)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures",
                                                                                               textRenderData.m_Textures,
                                                                                               textRenderData.m_TextureCount);
        else if(textRenderData.m_TextureCount == 0)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures", Material::GetDefaultTexture().get());
        else
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures",
                                                                                               textRenderData.m_Textures[0]);

        textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->Update();

        textRenderData.m_PreviousFrameTextureCount[textRenderData.m_BatchDrawCallIndex] = textRenderData.m_TextureCount;

        CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        textRenderData.m_Pipeline->Bind(commandBuffer);

        textRenderData.m_CurrentDescriptorSets[0] = textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][0].get();
        textRenderData.m_CurrentDescriptorSets[1] = textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1].get();

        textRenderData.m_IndexBuffer->SetCount(textRenderData.m_IndexCount);
        textRenderData.m_IndexBuffer->Bind(commandBuffer);

        if((int) textRenderData.m_VertexBuffers[currentFrame].size() - 1 < (int) textRenderData.m_BatchDrawCallIndex)
        {
            auto &vertexBuffer = textRenderData.m_VertexBuffers[currentFrame].emplace_back(
                    GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        uint32_t dataSize = (uint32_t) ((uint8_t*) textVertexBufferPtr - (uint8_t*) textVertexBufferBase[currentFrame]);
        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->SetData(dataSize,
                                                                                                   (void*) textVertexBufferBase[currentFrame]);

        // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();

        gEngine->GetRenderer()->BindDescriptorSets(textRenderData.m_Pipeline.get(), commandBuffer, 0,
                                                   textRenderData.m_CurrentDescriptorSets.data(), 2);
        gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::TRIANGLE, textRenderData.m_IndexCount);

        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Unbind();
        textRenderData.m_IndexBuffer->Unbind();

        textRenderData.m_Pipeline->End(commandBuffer);

        textVertexBufferPtr = textVertexBufferBase[currentFrame];
        textRenderData.m_IndexCount = 0;
        textRenderData.m_BatchDrawCallIndex++;

        if((int) textRenderData.m_VertexBuffers[currentFrame].size() - 1 < (int) textRenderData.m_BatchDrawCallIndex)
        {
            auto &vertexBuffer = textRenderData.m_VertexBuffers[currentFrame].emplace_back(
                    GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        textRenderData.m_TextureCount = 0;
        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Bind(
                GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), textRenderData.m_Pipeline.get());
        // m_TextBuffer = m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->GetPointer<TextVertexData>();
    }

//    void SceneRenderer::TextPass()
//    {
//
//        if(!m_Camera)
//            return;
//
//        auto textGroup = m_CurrentScene->GetRegistry().group<TextComponent>(entt::get<Transform>);
//        if(textGroup.size() == 0)
//            return;
//
//        PipelineDesc pipelineDesc;
//        pipelineDesc.shader = m_TextRendererData.m_Shader;
//        pipelineDesc.polygonMode = PolygonMode::FILL;
//        pipelineDesc.cullMode = CullMode::BACK;
//        pipelineDesc.isTransparencyEnabled = true;
//        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//        pipelineDesc.isClearTargets = false;
//        pipelineDesc.colourTargets[0] = m_MainTexture;
//        pipelineDesc.DebugName = "Text";
//
//        m_TextRendererData.m_Pipeline = Pipeline::Get(pipelineDesc);
//
//        uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();
//
//        if((int) m_TextRendererData.m_VertexBuffers[currentFrame].size() - 1 <
//           (int) m_TextRendererData.m_BatchDrawCallIndex)
//        {
//            auto &vertexBuffer = m_TextRendererData.m_VertexBuffers[currentFrame].emplace_back(
//                    GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//            vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//        }
//
//        m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->Bind(
//                GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_TextRendererData.m_Pipeline.get());
//        TextVertexBufferPtr = TextVertexBufferBase[currentFrame];
//        // m_TextBuffer = m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->GetPointer<TextVertexData>();
//
//        auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
//        m_TextRendererData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
//        m_TextRendererData.m_DescriptorSet[0][0]->Update();
//
//        m_TextRendererData.m_TextureCount = 0;
//        for(auto entity: textGroup)
//        {
//            const auto &[textComp, trans] = textGroup.get<TextComponent, Transform>(entity);
//
//            glm::mat4 transform = trans.GetWorldMatrix();
//            m_Stats.NumRenderedObjects++;
//
//            if(m_TextRendererData.m_IndexCount >= m_TextRendererData.m_Limits.IndiciesSize)
//                TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
//
//            int textureIndex = -1;
//            auto &string = textComp.TextString;
//            auto font = textComp.FontHandle ? textComp.FontHandle : Font::GetDefaultFont();
//            float lineHeightOffset = 0.0f;
//            float kerningOffset = 0.0f;
//
//            float maxWidth = textComp.MaxWidth;
//            auto colour = textComp.Colour;
//            float lineSpacing = textComp.LineSpacing;
//            float kerning = textComp.Kerning;
//            auto outlineColour = textComp.OutlineColour;
//            auto outlineWidth = textComp.OutlineWidth;
//
//            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
//            if(!fontAtlas)
//                continue;
//
//            for(uint32_t i = 0; i < m_TextRendererData.m_TextureCount; i++)
//            {
//                if(m_TextRendererData.m_Textures[i] == fontAtlas.get())
//                {
//                    textureIndex = int(i);
//                    break;
//                }
//            }
//
//            if(textureIndex == -1)
//            {
//                textureIndex = (int) m_TextRendererData.m_TextureCount;
//                m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
//                m_TextRendererData.m_TextureCount++;
//            }
//
//            auto &fontGeometry = font->GetMSDFData()->FontGeometry;
//            const auto &metrics = fontGeometry.getMetrics();
//
//            {
//                double x = 0.0;
//                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
//                double y = 0.0;
//                for(int i = 0; i < string.size(); i++)
//                {
//                    char32_t character = string[i];
//
//                    if(character == '\r')
//                        continue;
//
//                    if(character == '\n')
//                    {
//                        x = 0;
//                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
//                        continue;
//                    }
//
//                    if(character == '\t')
//                    {
//                        auto glyph = fontGeometry.getGlyph('a');
//                        double advance = glyph->getAdvance();
//                        x += 4 * fsScale * advance + kerningOffset;
//                        continue;
//                    }
//
//                    auto glyph = fontGeometry.getGlyph(character);
//                    if(!glyph)
//                        glyph = fontGeometry.getGlyph('?');
//                    if(!glyph)
//                        continue;
//
//                    double l, b, r, t;
//                    glyph->getQuadAtlasBounds(l, b, r, t);
//
//                    double pl, pb, pr, pt;
//                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);
//
//                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
//                    pl += x, pb += y, pr += x, pt += y;
//
//                    double texelWidth = 1. / fontAtlas->GetWidth();
//                    double texelHeight = 1. / fontAtlas->GetHeight();
//                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;
//
//                    {
//                        TextVertexBufferPtr->vertex = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
//                        TextVertexBufferPtr->colour = colour;
//                        TextVertexBufferPtr->uv = {l, b};
//                        TextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        TextVertexBufferPtr->outlineColour = outlineColour;
//                        TextVertexBufferPtr++;
//
//                        TextVertexBufferPtr->vertex = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
//                        TextVertexBufferPtr->colour = colour;
//                        TextVertexBufferPtr->uv = {r, b};
//                        TextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        TextVertexBufferPtr->outlineColour = outlineColour;
//                        TextVertexBufferPtr++;
//
//                        TextVertexBufferPtr->vertex = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
//                        TextVertexBufferPtr->colour = colour;
//                        TextVertexBufferPtr->uv = {r, t};
//                        TextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        TextVertexBufferPtr->outlineColour = outlineColour;
//                        TextVertexBufferPtr++;
//
//                        TextVertexBufferPtr->vertex = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
//                        TextVertexBufferPtr->colour = colour;
//                        TextVertexBufferPtr->uv = {l, t};
//                        TextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        TextVertexBufferPtr->outlineColour = outlineColour;
//                        TextVertexBufferPtr++;
//                    }
//
//                    m_TextRendererData.m_IndexCount += 6;
//
//                    double advance = glyph->getAdvance();
//                    fontGeometry.getAdvance(advance, character, string[i + 1]);
//                    x += fsScale * advance + kerningOffset;
//                }
//            }
//        }
//
//        if(m_TextRendererData.m_IndexCount == 0)
//        {
//            // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
//            return;
//        }
//
//        TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
//
//        // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
//    }

//    void SceneRenderer::DebugPass()
//    {
//        if(!m_Camera || !m_CameraTransform)
//            return;
//
//        // Loop twice for depth test and no depth test
//        for(int i = 0; i < 2; i++)
//        {
//            bool depthTest = i == 1;
//
//            auto &lines = DebugRenderer::GetInstance()->GetLines(depthTest);
//            auto &thickLines = DebugRenderer::GetInstance()->GetThickLines(depthTest);
//            auto &triangles = DebugRenderer::GetInstance()->GetTriangles(depthTest);
//            auto &points = DebugRenderer::GetInstance()->GetPoints(depthTest);
//
//            auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
//
//            if(!lines.empty())
//            {
//                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
//                m_DebugDrawData.m_LineDescriptorSet[0]->Update();
//
//                CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
//
//                PipelineDesc pipelineDesc;
//                pipelineDesc.shader = m_DebugDrawData.m_LineShader;
//
//                pipelineDesc.polygonMode = PolygonMode::FILL;
//                pipelineDesc.cullMode = CullMode::BACK;
//                pipelineDesc.isTransparencyEnabled = false;
//                pipelineDesc.isClearTargets = false;
//                pipelineDesc.drawType = DrawType::LINES;
//                pipelineDesc.colourTargets[0] = m_MainTexture;
//                pipelineDesc.DebugName = "Debug-Lines";
//
//                if(depthTest)
//                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
//
//                auto pipeline = Pipeline::Get(pipelineDesc);
//
//                pipeline->Bind(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                if((int) m_DebugDrawData.m_LineVertexBuffers.size() - 1 <
//                   (int) m_DebugDrawData.m_LineBatchDrawCallIndex)
//                {
//                    auto &vertexBuffer = m_DebugDrawData.m_LineVertexBuffers.emplace_back(
//                            GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                    vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//                }
//
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(
//                        GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), pipeline.get());
//                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0];
//
//                // m_DebugDrawData.m_LineBuffer = m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->GetPointer<LineVertexData>();
//
//                for(auto &line: lines)
//                {
//                    if(m_DebugDrawData.LineIndexCount >= m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads)
//                        break;
//
//                    m_DebugDrawData.m_LineBuffer->vertex = line.p1;
//                    m_DebugDrawData.m_LineBuffer->colour = line.col;
//                    m_DebugDrawData.m_LineBuffer++;
//
//                    m_DebugDrawData.m_LineBuffer->vertex = line.p2;
//                    m_DebugDrawData.m_LineBuffer->colour = line.col;
//                    m_DebugDrawData.m_LineBuffer++;
//
//                    m_DebugDrawData.LineIndexCount += 2;
//                }
//
//                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->ReleasePointer();
//                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
//                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());
//
//                m_DebugDrawData.m_LineIndexBuffer->SetCount(m_DebugDrawData.LineIndexCount);
//                m_DebugDrawData.m_LineIndexBuffer->Bind(commandBuffer);
//
//                uint32_t dataSize = (uint32_t) ((uint8_t*) m_DebugDrawData.m_LineBuffer -
//                                                (uint8_t*) m_LineBufferBase[0]);
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->SetData(dataSize,
//                                                                                                       (void*) m_LineBufferBase[0]);
//
//                auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
//                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &desc, 1);
//                gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::LINES, m_DebugDrawData.LineIndexCount);
//
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
//                m_DebugDrawData.m_LineIndexBuffer->Unbind();
//
//                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0];
//                m_DebugDrawData.LineIndexCount = 0;
//
//                pipeline->End(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                m_DebugDrawData.m_LineBatchDrawCallIndex++;
//            }
//
//            if(!thickLines.empty())
//            {
//                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
//                m_DebugDrawData.m_LineDescriptorSet[0]->Update();
//
//                CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
//
//                PipelineDesc pipelineDesc;
//                pipelineDesc.shader = m_DebugDrawData.m_LineShader;
//
//                pipelineDesc.polygonMode = PolygonMode::FILL;
//                pipelineDesc.cullMode = CullMode::BACK;
//                pipelineDesc.isTransparencyEnabled = false;
//                pipelineDesc.isClearTargets = false;
//                pipelineDesc.drawType = DrawType::LINES;
//                pipelineDesc.colourTargets[0] = m_MainTexture;
//                pipelineDesc.lineWidth = 2.0f;
//                pipelineDesc.DebugName = "Debug-ThickLines";
//
//                if(depthTest)
//                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
//
//                auto pipeline = Pipeline::Get(pipelineDesc);
//
//                pipeline->Bind(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                if((int) m_DebugDrawData.m_LineVertexBuffers.size() - 1 <
//                   (int) m_DebugDrawData.m_LineBatchDrawCallIndex)
//                {
//                    auto &vertexBuffer = m_DebugDrawData.m_LineVertexBuffers.emplace_back(
//                            GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                    vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//                }
//
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(
//                        GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), pipeline.get());
//                // m_DebugDrawData.m_LineBuffer = m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->GetPointer<LineVertexData>();
//                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0];
//
//                for(auto &line: thickLines)
//                {
//                    if(m_DebugDrawData.LineIndexCount >= m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads)
//                        break;
//
//                    m_DebugDrawData.m_LineBuffer->vertex = line.p1;
//                    m_DebugDrawData.m_LineBuffer->colour = line.col;
//                    m_DebugDrawData.m_LineBuffer++;
//
//                    m_DebugDrawData.m_LineBuffer->vertex = line.p2;
//                    m_DebugDrawData.m_LineBuffer->colour = line.col;
//                    m_DebugDrawData.m_LineBuffer++;
//
//                    m_DebugDrawData.LineIndexCount += 2;
//                }
//
//                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->ReleasePointer();
//                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
//
//                m_DebugDrawData.m_LineIndexBuffer->SetCount(m_DebugDrawData.LineIndexCount);
//
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(commandBuffer,
//                                                                                                    pipeline.get());
//                uint32_t dataSize = (uint32_t) ((uint8_t*) m_DebugDrawData.m_LineBuffer -
//                                                (uint8_t*) m_LineBufferBase[0]);
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->SetData(dataSize,
//                                                                                                       (void*) m_LineBufferBase[0]);
//
//                m_DebugDrawData.m_LineIndexBuffer->Bind(commandBuffer);
//                auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
//                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &desc, 1);
//                gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::LINES, m_DebugDrawData.LineIndexCount);
//
//                m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
//                m_DebugDrawData.m_LineIndexBuffer->Unbind();
//
//                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0];
//                m_DebugDrawData.LineIndexCount = 0;
//
//                pipeline->End(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                m_DebugDrawData.m_LineBatchDrawCallIndex++;
//            }
//
//            if(!points.empty())
//            {
//                m_DebugDrawData.m_PointDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
//                m_DebugDrawData.m_PointDescriptorSet[0]->Update();
//
//                CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
//
//                PipelineDesc pipelineDesc;
//                pipelineDesc.shader = m_DebugDrawData.m_PointShader;
//
//                pipelineDesc.polygonMode = PolygonMode::FILL;
//                pipelineDesc.cullMode = CullMode::BACK;
//                pipelineDesc.isTransparencyEnabled = true;
//                pipelineDesc.drawType = DrawType::TRIANGLE;
//                pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//                pipelineDesc.colourTargets[0] = m_MainTexture;
//                pipelineDesc.DebugName = "Debug-Points";
//
//                if(depthTest)
//                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
//
//                auto pipeline = Pipeline::Get(pipelineDesc);
//
//                pipeline->Bind(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                if((int) m_DebugDrawData.m_PointVertexBuffers.size() - 1 <
//                   (int) m_DebugDrawData.m_PointBatchDrawCallIndex)
//                {
//                    auto &vertexBuffer = m_DebugDrawData.m_PointVertexBuffers.emplace_back(
//                            GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                    vertexBuffer->Resize(RENDERER_POINT_BUFFER_SIZE);
//                }
//
//                m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(
//                        GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), pipeline.get());
//                m_DebugDrawData.m_PointBuffer = m_PointBufferBase[0]; // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->GetPointer<PointVertexData>();
//
//                for(auto &pointInfo: points)
//                {
//                    glm::vec3 right = pointInfo.size * m_CameraTransform->GetRightDirection();
//                    glm::vec3 up = pointInfo.size * m_CameraTransform->GetUpDirection();
//
//                    m_DebugDrawData.m_PointBuffer->vertex =
//                            pointInfo.p1 - right - up; // + glm::vec3(-pointInfo.size, -pointInfo.size, 0.0f));
//                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
//                    m_DebugDrawData.m_PointBuffer->size = {pointInfo.size, 0.0f};
//                    m_DebugDrawData.m_PointBuffer->uv = {-1.0f, -1.0f};
//                    m_DebugDrawData.m_PointBuffer++;
//
//                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right -
//                                                            up; //(pointInfo.p1 + glm::vec3(pointInfo.size, -pointInfo.size, 0.0f));
//                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
//                    m_DebugDrawData.m_PointBuffer->size = {pointInfo.size, 0.0f};
//                    m_DebugDrawData.m_PointBuffer->uv = {1.0f, -1.0f};
//                    m_DebugDrawData.m_PointBuffer++;
//
//                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right +
//                                                            up; //(pointInfo.p1 + glm::vec3(pointInfo.size, pointInfo.size, 0.0f));
//                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
//                    m_DebugDrawData.m_PointBuffer->size = {pointInfo.size, 0.0f};
//                    m_DebugDrawData.m_PointBuffer->uv = {1.0f, 1.0f};
//                    m_DebugDrawData.m_PointBuffer++;
//
//                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right +
//                                                            up; // (pointInfo.p1 + glm::vec3(-pointInfo.size, pointInfo.size, 0.0f));
//                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
//                    m_DebugDrawData.m_PointBuffer->size = {pointInfo.size, 0.0f};
//                    m_DebugDrawData.m_PointBuffer->uv = {-1.0f, 1.0f};
//                    m_DebugDrawData.m_PointBuffer++;
//
//                    m_DebugDrawData.PointIndexCount += 6;
//                }
//
//                // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->ReleasePointer();
//                m_DebugDrawData.m_PointIndexBuffer->SetCount(m_DebugDrawData.PointIndexCount);
//                m_DebugDrawData.m_PointIndexBuffer->Bind(commandBuffer);
//
//                uint32_t dataSize = (uint32_t) ((uint8_t*) m_DebugDrawData.m_PointBuffer -
//                                                (uint8_t*) m_PointBufferBase[0]);
//                m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->SetData(dataSize,
//                                                                                                         (void*) m_PointBufferBase[0]);
//
//                auto* desc = m_DebugDrawData.m_PointDescriptorSet[0].get();
//                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &desc, 1);
//                gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_DebugDrawData.PointIndexCount);
//
//                m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Unbind();
//                m_DebugDrawData.m_PointIndexBuffer->Unbind();
//
//                m_DebugDrawData.m_PointBuffer = m_PointBufferBase[0];
//                m_DebugDrawData.PointIndexCount = 0;
//
//                pipeline->End(GET_SWAP_CHAIN()->GetCurrentCommandBuffer());
//
//                m_DebugDrawData.m_PointBatchDrawCallIndex++;
//            }
//
//            if(!triangles.empty())
//            {
//                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
//                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->Update();
//                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1]->Update();
//
//                PipelineDesc pipelineDesc;
//                pipelineDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader;
//
//                pipelineDesc.polygonMode = PolygonMode::FILL;
//                pipelineDesc.cullMode = CullMode::BACK;
//                pipelineDesc.isTransparencyEnabled = true;
//                pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//                pipelineDesc.isClearTargets = false;
//                pipelineDesc.colourTargets[0] = m_MainTexture;
//                pipelineDesc.isDepthBiasEnabled = true;
//                pipelineDesc.depthBiasConstantFactor = -1.25f;
//                pipelineDesc.depthBiasSlopeFactor = -1.75f;
//                pipelineDesc.DebugName = "Debug-Triangles";
//
//                if(depthTest)
//                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
//
//                auto pipeline = Pipeline::Get(pipelineDesc);
//
//                m_DebugDrawData.m_Renderer2DData.m_TextureCount = 0;
//                uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();
//
//                if((int) m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].size() - 1 <
//                   (int) m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex)
//                {
//                    auto &vertexBuffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(
//                            GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                    vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//                }
//
//                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Bind(
//                        GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), pipeline.get());
//                // m_DebugDrawData.m_Renderer2DData.m_Buffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();
//                m_DebugDrawData.m_Renderer2DData.m_Buffer = m_QuadBufferBase[currentFrame];
//
//                for(auto &triangleInfo: triangles)
//                {
//                    m_Stats.NumRenderedObjects++;
//
//                    float textureSlot = 0.0f;
//
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p1;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = {0.0f, 0.0f};
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p2;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = {0.0f, 0.0f};
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p3;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = {0.0f, 0.0f};
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = glm::vec2(textureSlot, 0.0f);
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
//                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
//                    m_DebugDrawData.m_Renderer2DData.m_IndexCount += 3;
//                }
//
//                CommandBuffer* commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();
//
//                pipeline->Bind(commandBuffer);
//
//                m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets[0] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0].get();
//                m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets[1] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1].get();
//
//                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->SetCount(m_DebugDrawData.m_Renderer2DData.m_IndexCount);
//                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);
//
//                // m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();
//
//                uint32_t dataSize = (uint32_t) ((uint8_t*) m_DebugDrawData.m_Renderer2DData.m_Buffer -
//                                                (uint8_t*) m_QuadBufferBase[currentFrame]);
//                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->SetData(
//                        dataSize, (void*) m_QuadBufferBase[currentFrame]);
//
//                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0,
//                                                           m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets.data(),
//                                                           2);
//                gEngine->GetRenderer()->DrawIndexed(commandBuffer, DrawType::TRIANGLE,
//                                                    m_DebugDrawData.m_Renderer2DData.m_IndexCount);
//
//                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
//                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Unbind();
//
//                pipeline->End(commandBuffer);
//
//                m_DebugDrawData.m_Renderer2DData.m_Buffer = m_QuadBufferBase[currentFrame];
//                m_DebugDrawData.m_Renderer2DData.m_IndexCount = 0;
//            }
//        }
//
//        m_DebugDrawData.m_PointBatchDrawCallIndex = 0;
//        m_DebugDrawData.m_LineBatchDrawCallIndex = 0;
//
//        // Text Pass
//        auto &dtDebugText = DebugRenderer::GetInstance()->GetDebugText();
//        if(!dtDebugText.empty())
//        {
//            PipelineDesc pipelineDesc;
//            pipelineDesc.shader = m_DebugTextRendererData.m_Shader;
//            pipelineDesc.polygonMode = PolygonMode::FILL;
//            pipelineDesc.cullMode = CullMode::BACK;
//            pipelineDesc.isTransparencyEnabled = true;
//            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//            pipelineDesc.isClearTargets = false;
//            pipelineDesc.colourTargets[0] = m_MainTexture;
//            pipelineDesc.DebugName = "Debug-TextDT";
//            pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
//            m_DebugTextRendererData.m_Pipeline = Pipeline::Get(pipelineDesc);
//
//            uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();
//
//            while((int) m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 <
//                  (int) m_DebugTextRendererData.m_BatchDrawCallIndex)
//            {
//                auto &vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(
//                        GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//            }
//
//            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(
//                    GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
//            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
//            auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
//
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO",
//                                                                                                                 "projView",
//                                                                                                                 &projView);
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();
//
//            m_DebugTextRendererData.m_TextureCount = 0;
//            m_Stats.NumRenderedObjects++;
//
//            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
//                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//
//            int textureIndex = -1;
//            // auto& string = textComp.TextString;
//            auto font = Font::GetDefaultFont();
//            float lineHeightOffset = 0.0f;
//            float kerningOffset = 0.0f;
//            float outlineWidth = 0.0f;
//
//            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
//            if(!fontAtlas)
//                return; // TODO
//
//            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
//            {
//                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
//                {
//                    textureIndex = int(i);
//                    break;
//                }
//            }
//
//            if(textureIndex == -1)
//            {
//                textureIndex = (int) m_DebugTextRendererData.m_TextureCount;
//                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
//                m_DebugTextRendererData.m_TextureCount++;
//            }
//
//            auto &fontGeometry = font->GetMSDFData()->FontGeometry;
//            const auto &metrics = fontGeometry.getMetrics();
//
//            for(auto &text: dtDebugText)
//            {
//                double x = 0.0;
//                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
//                double y = 0.0;
//                for(int i = 0; i < text.text.size(); i++)
//                {
//                    char32_t character = (char32_t) text.text[i];
//                    glm::vec4 pos = text.Position;
//                    glm::vec4 colour = text.colour;
//                    glm::vec4 outlineColour = text.colour;
//                    float size = text.Size;
//
//                    if(character == '\r')
//                        continue;
//
//                    if(character == '\n')
//                    {
//                        x = 0;
//                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
//                        continue;
//                    }
//
//                    if(character == '\t')
//                    {
//                        auto glyph = fontGeometry.getGlyph('a');
//                        double advance = glyph->getAdvance();
//                        x += 4 * fsScale * advance + kerningOffset;
//                        continue;
//                    }
//
//                    auto glyph = fontGeometry.getGlyph(character);
//                    if(!glyph)
//                        glyph = fontGeometry.getGlyph('?');
//                    if(!glyph)
//                        continue;
//
//                    double l, b, r, t;
//                    glyph->getQuadAtlasBounds(l, b, r, t);
//
//                    double pl, pb, pr, pt;
//                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);
//
//                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
//                    pl += x, pb += y, pr += x, pt += y;
//
//                    double texelWidth = 1. / fontAtlas->GetWidth();
//                    double texelHeight = 1. / fontAtlas->GetHeight();
//                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;
//
//                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) *
//                                          glm::toMat4(m_CameraTransform->GetLocalOrientation()) *
//                                          glm::scale(glm::mat4(1.0), glm::vec3(size / 10.0f));
//
//                    {
//                        NekoEngine_PROFILE_SCOPE("Set text buffer data");
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//                    }
//
//                    m_DebugTextRendererData.m_IndexCount += 6;
//
//                    double advance = glyph->getAdvance();
//
//                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
//                    x += fsScale * advance + kerningOffset;
//                }
//            }
//
//            if(m_DebugTextRendererData.m_IndexCount == 0)
//            {
//                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
//                return;
//            }
//
//            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//        }
//
//        auto &ndtDebugText = DebugRenderer::GetInstance()->GetDebugTextNDT();
//        if(!ndtDebugText.empty())
//        {
//            PipelineDesc pipelineDesc;
//            pipelineDesc.shader = m_DebugTextRendererData.m_Shader;
//            pipelineDesc.polygonMode = PolygonMode::FILL;
//            pipelineDesc.cullMode = CullMode::BACK;
//            pipelineDesc.isTransparencyEnabled = true;
//            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//            pipelineDesc.isClearTargets = false;
//            pipelineDesc.colourTargets[0] = m_MainTexture;
//            pipelineDesc.DebugName = "Debug-TextNDT";
//            m_DebugTextRendererData.m_Pipeline = Pipeline::Get(pipelineDesc);
//
//            uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();
//
//            while((int) m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 <
//                  (int) m_DebugTextRendererData.m_BatchDrawCallIndex)
//            {
//                auto &vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(
//                        GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//            }
//
//            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(
//                    GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
//            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
//            auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
//            DebugRenderer::GetInstance()->SetProjView(projView);
//            DebugRenderer::SortLists();
//
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO",
//                                                                                                                 "projView",
//                                                                                                                 &projView);
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();
//
//            m_DebugTextRendererData.m_TextureCount = 0;
//            m_Stats.NumRenderedObjects++;
//
//            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
//                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//
//            int textureIndex = -1;
//            // auto& string = textComp.TextString;
////            auto font = Font::GetDefaultFont();
//            float lineHeightOffset = 0.0f;
//            float kerningOffset = 0.0f;
//            float outlineWidth = 0.0f;
//
//            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
//            if(!fontAtlas)
//                return; // TODO
//
//            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
//            {
//                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
//                {
//                    textureIndex = int(i);
//                    break;
//                }
//            }
//
//            if(textureIndex == -1)
//            {
//                textureIndex = (int) m_DebugTextRendererData.m_TextureCount;
//                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
//                m_DebugTextRendererData.m_TextureCount++;
//            }
//
//            auto &fontGeometry = font->GetMSDFData()->FontGeometry;
//            const auto &metrics = fontGeometry.getMetrics();
//
//            for(auto &text: ndtDebugText)
//            {
//                double x = 0.0;
//                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
//                double y = 0.0;
//                for(int i = 0; i < text.text.size(); i++)
//                {
//                    char32_t character = (char32_t) text.text[i];
//                    glm::vec4 pos = text.Position;
//                    glm::vec4 colour = text.colour;
//                    glm::vec4 outlineColour = text.colour;
//                    float size = text.Size;
//                    if(character == '\r')
//                        continue;
//
//                    if(character == '\n')
//                    {
//                        x = 0;
//                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
//                        continue;
//                    }
//
//                    if(character == '\t')
//                    {
//                        auto glyph = fontGeometry.getGlyph('a');
//                        double advance = glyph->getAdvance();
//                        x += 4 * fsScale * advance + kerningOffset;
//                        continue;
//                    }
//
//                    auto glyph = fontGeometry.getGlyph(character);
//                    if(!glyph)
//                        glyph = fontGeometry.getGlyph('?');
//                    if(!glyph)
//                        continue;
//
//                    double l, b, r, t;
//                    glyph->getQuadAtlasBounds(l, b, r, t);
//
//                    double pl, pb, pr, pt;
//                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);
//
//                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
//                    pl += x, pb += y, pr += x, pt += y;
//
//                    double texelWidth = 1. / fontAtlas->GetWidth();
//                    double texelHeight = 1. / fontAtlas->GetHeight();
//                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;
//
//                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) *
//                                          glm::toMat4(m_CameraTransform->localOrientation) *
//                                          glm::scale(glm::mat4(1.0), glm::vec3(size / 10.0f));
//
//                    {
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//                    }
//
//                    m_DebugTextRendererData.m_IndexCount += 6;
//
//                    double advance = glyph->getAdvance();
//
//                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
//                    x += fsScale * advance + kerningOffset;
//                }
//            }
//
//            if(m_DebugTextRendererData.m_IndexCount == 0)
//            {
//                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
//                return;
//            }
//
//            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//        }
//
//        glm::mat4 csProjection = glm::ortho(0.0f, (float) m_MainTexture->GetWidth(), 0.0f,
//                                            (float) m_MainTexture->GetHeight(), -100.0f, 100.0f);
//        auto projView = m_Camera->GetProjectionMatrix();
//        projView = glm::mat4(1.0f);
//
//        float scale = 1.0f;
//        float aspectRatio = (float) m_MainTexture->GetWidth() / (float) m_MainTexture->GetHeight();
//        projView = glm::ortho(-aspectRatio * scale, aspectRatio * scale, -scale, scale, -10.0f, 10.0f);
//
//        DebugRenderer::GetInstance()->SetProjView(projView);
//        DebugRenderer::SortLists();
//
//        auto &csDebugText = DebugRenderer::GetInstance()->GetDebugTextCS();
//        if(!csDebugText.empty())
//        {
//            PipelineDesc pipelineDesc;
//            pipelineDesc.shader = m_DebugTextRendererData.m_Shader;
//            pipelineDesc.polygonMode = PolygonMode::FILL;
//            pipelineDesc.cullMode = CullMode::BACK;
//            pipelineDesc.isTransparencyEnabled = true;
//            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
//            pipelineDesc.isClearTargets = false;
//            pipelineDesc.colourTargets[0] = m_MainTexture;
//            pipelineDesc.DebugName = "Debug-TextCS";
//            m_DebugTextRendererData.m_Pipeline = Pipeline::Get(pipelineDesc);
//
//            uint32_t currentFrame = GET_SWAP_CHAIN()->GetCurrentBufferIndex();
//
//            while((int) m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 <
//                  (int) m_DebugTextRendererData.m_BatchDrawCallIndex)
//            {
//                auto &vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(
//                        GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::DYNAMIC));
//                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
//            }
//
//            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(
//                    GET_SWAP_CHAIN()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
//            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
//
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO",
//                                                                                                                 "projView",
//                                                                                                                 &projView);
//            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();
//
//            m_DebugTextRendererData.m_TextureCount = 0;
//            m_Stats.NumRenderedObjects++;
//
//            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
//                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//
//            int textureIndex = -1;
//            // auto& string = textComp.TextString;
////            auto font = Font::GetDefaultFont();
//            float lineHeightOffset = 0.0f;
//            float kerningOffset = 0.0f;
//            float outlineWidth = 0.0f;
//
//            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
//            if(!fontAtlas)
//                return; // TODO
//
//            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
//            {
//                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
//                {
//                    textureIndex = int(i);
//                    break;
//                }
//            }
//
//            if(textureIndex == -1)
//            {
//                textureIndex = (int) m_DebugTextRendererData.m_TextureCount;
//                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
//                m_DebugTextRendererData.m_TextureCount++;
//            }
//
//            auto &fontGeometry = font->GetMSDFData()->FontGeometry;
//            const auto &metrics = fontGeometry.getMetrics();
//
//            for(auto &text: csDebugText)
//            {
//                double x = 0.0;
//                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
//                double y = 0.0;
//                for(int i = 0; i < text.text.size(); i++)
//                {
//                    char32_t character = (char32_t) text.text[i];
//                    glm::vec4 pos = text.Position;
//                    glm::vec4 colour = text.colour;
//                    glm::vec4 outlineColour = text.colour;
//
//                    if(character == '\r')
//                        continue;
//
//                    if(character == '\n')
//                    {
//                        x = 0;
//                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
//                        continue;
//                    }
//
//                    if(character == '\t')
//                    {
//                        auto glyph = fontGeometry.getGlyph('a');
//                        double advance = glyph->getAdvance();
//                        x += 4 * fsScale * advance + kerningOffset;
//                        continue;
//                    }
//
//                    auto glyph = fontGeometry.getGlyph(character);
//                    if(!glyph)
//                        glyph = fontGeometry.getGlyph('?');
//                    if(!glyph)
//                        continue;
//
//                    double l, b, r, t;
//                    glyph->getQuadAtlasBounds(l, b, r, t);
//
//                    double pl, pb, pr, pt;
//                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);
//
//                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
//                    pl += x, pb += y, pr += x, pt += y;
//
//                    double texelWidth = 1. / fontAtlas->GetWidth();
//                    double texelHeight = 1. / fontAtlas->GetHeight();
//                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;
//
//                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) *
//                                          glm::scale(glm::mat4(1.0), glm::vec3(0.05f, 0.05f, 0.05f));
//
//                    {
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, b};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {r, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//
//                        DebugTextVertexBufferPtr->vertex = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
//                        DebugTextVertexBufferPtr->colour = colour;
//                        DebugTextVertexBufferPtr->uv = {l, t};
//                        DebugTextVertexBufferPtr->tid = glm::vec2(textureIndex, outlineWidth);
//                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
//                        DebugTextVertexBufferPtr++;
//                    }
//
//                    m_DebugTextRendererData.m_IndexCount += 6;
//
//                    double advance = glyph->getAdvance();
//
//                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
//                    x += fsScale * advance + kerningOffset;
//                }
//            }
//
//            if(m_DebugTextRendererData.m_IndexCount == 0)
//            {
//                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
//                return;
//            }
//
//            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
//        }
//    }

    void
    SceneRenderer::CreateCubeMap(const std::string &filePath, const glm::vec4 &params, SharedPtr<TextureCube> &outEnv,
                                 SharedPtr<TextureCube> &outIrr)
    {
        // Create shader and pipeline
        // Create Empty Cube Map
        auto environmentMap = GET_RHI_FACTORY()->CreateTextureCube(
                gEngine->GetRenderer()->GetRenderConfig().EnvironmentMapSize, nullptr, true);
        auto environmentMapFiltered = GET_RHI_FACTORY()->CreateTextureCube(
                gEngine->GetRenderer()->GetRenderConfig().EnvironmentMapSize, nullptr, true);
        auto irradianceMap = GET_RHI_FACTORY()->CreateTextureCube(
                gEngine->GetRenderer()->GetRenderConfig().IrradianceMapSize, nullptr, true);

        auto commandBuffer = GET_RHI_FACTORY()->CreateCommandBuffer();
        commandBuffer->Init(true);
        commandBuffer->BeginRecording();
        {

            auto shader = GET_SHADER_LIB()->GetResource("CreateEnvironmentMap");

            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = shader.get();
            auto descriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

            Texture* hdri = nullptr;

            if(!filePath.empty())
            {
                // Load hdri image
                TextureDesc textureParams;
                textureParams.srgb = false;
                hdri = GET_RHI_FACTORY()->CreateTexture2DFromFile("Environment", filePath, textureParams);
                descriptorSet->SetTexture("u_Texture", hdri);
            }
            else
            {
                descriptorSet->SetTexture("u_Texture", Material::GetDefaultTexture().get());
            }

            descriptorSet->SetUniform("UniformBuffer", "u_Parameters", (void*) &params);
            descriptorSet->Update(commandBuffer);

            PipelineDesc pipelineDesc{};
            pipelineDesc.shader = shader;
            pipelineDesc.polygonMode = PolygonMode::FILL;
            pipelineDesc.cullMode = CullMode::BACK;
            pipelineDesc.isTransparencyEnabled = false;
            pipelineDesc.cubeMapTarget = environmentMap;
            pipelineDesc.DebugName = "Create Cubemap";

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Pipeline::Get(pipelineDesc);
                pipeline->Bind(commandBuffer, i);

                auto &pushConstants = shader->GetPushConstants();
                if(!pushConstants.empty())
                {
                    auto &pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*) &i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

                pipeline->End(commandBuffer);
            }
        }

        // Generate Mips
        {
            environmentMap->GenerateMipMaps(commandBuffer);

            auto shader = GET_SHADER_LIB()->GetResource("EnvironmentMipFilter");

            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = shader.get();
            auto descriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
            descriptorSet->SetTexture("u_Texture", environmentMap);
            descriptorSet->Update(commandBuffer);

            const float deltaRoughness =
                    1.0f / Maths::Max((float) environmentMapFiltered->GetMipMapLevels() - 1.0f, 1.0f);

            for(uint32_t mip = 0; mip < environmentMapFiltered->GetMipMapLevels(); mip++)
            {
                PipelineDesc pipelineDesc{};
                pipelineDesc.shader = shader;
                pipelineDesc.polygonMode = PolygonMode::FILL;
                pipelineDesc.cullMode = CullMode::BACK;
                pipelineDesc.isTransparencyEnabled = false;
                pipelineDesc.cubeMapTarget = environmentMapFiltered;
                pipelineDesc.DebugName = "Mip Generation";

                for(uint32_t i = 0; i < 6; i++)
                {
                    pipelineDesc.cubeMapIndex = i;
                    pipelineDesc.mipIndex = mip;
                    auto pipeline = Pipeline::Get(pipelineDesc);
                    pipeline->Bind(commandBuffer, i);

                    auto &pushConstants = shader->GetPushConstants();
                    if(!pushConstants.empty())
                    {
                        float roughness = mip * deltaRoughness;
                        roughness = Maths::Max(roughness, 0.05f);

                        auto &pushConstant = shader->GetPushConstants()[0];
                        pushConstant.SetValue("Roughness", (void*) &roughness);
                        pushConstant.SetValue("cubeFaceIndex", (void*) &i);
                        shader->BindPushConstants(commandBuffer, pipeline.get());
                    }

                    auto set = descriptorSet.get();
                    gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                    gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

                    pipeline->End(commandBuffer);
                }
            }
        }

        {
            auto shader = GET_SHADER_LIB()->GetResource("EnvironmentIrradiance");

            DescriptorDesc descriptorDesc{};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = shader.get();
            auto descriptorSet = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));

            descriptorSet->SetTexture("u_Texture", environmentMapFiltered);

            uint32_t samples = 512;
            descriptorSet->SetUniform("UniformBuffer", "Samples", (void*) &samples);
            descriptorSet->Update(commandBuffer);

            PipelineDesc pipelineDesc{};
            pipelineDesc.shader = shader;
            pipelineDesc.polygonMode = PolygonMode::FILL;
            pipelineDesc.cullMode = CullMode::BACK;
            pipelineDesc.isTransparencyEnabled = false;
            pipelineDesc.cubeMapTarget = irradianceMap;

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Pipeline::Get(pipelineDesc);
                pipeline->Bind(commandBuffer, i);

                auto &pushConstants = shader->GetPushConstants();
                if(!pushConstants.empty())
                {
                    auto &pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*) &i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                gEngine->GetRenderer()->BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                gEngine->GetRenderer()->Draw(commandBuffer, DrawType::TRIANGLE, 3);

                pipeline->End(commandBuffer);
            }
        }

        irradianceMap->GenerateMipMaps(commandBuffer);

        commandBuffer->EndRecording();
        commandBuffer->Submit();

        delete environmentMap;
        delete commandBuffer;
        outEnv = SharedPtr<TextureCube>(environmentMapFiltered);
        outIrr = SharedPtr<TextureCube>(irradianceMap);
    }

    void SceneRenderer::DebugPass()
    {
        //TODO: Implement Debug Pass
    }

    void SceneRenderer::TextPass()
    {

    }

    void SceneRenderer::SharpenPass()
    {

    }

}
