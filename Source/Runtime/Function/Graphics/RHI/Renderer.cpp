#include "Renderer.h"
#include "RHIFactory.h"
#include "EmbeddedShaders.h"


namespace NekoEngine
{
    RenderAPICapabilities Renderer::capabilities = RenderAPICapabilities();

    void Renderer::LoadEngineShaders(bool loadEmbeddedShaders)
    {
        if(loadEmbeddedShaders)
        {
            LOG("Loading embedded Shaders...");
            shaderLibrary->AddResource("Skybox", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Skyboxvertspv.data(), spirv_Skyboxvertspv_size, spirv_Skyboxfragspv.data(), spirv_Skyboxfragspv_size)));
            shaderLibrary->AddResource("ForwardPBR", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_ForwardPBRfragspv.data(), spirv_ForwardPBRfragspv_size)));
            shaderLibrary->AddResource("Shadow", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Shadowvertspv.data(), spirv_Shadowvertspv_size, spirv_Shadowfragspv.data(), spirv_Shadowfragspv_size)));
            shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Batch2DPointvertspv.data(), spirv_Batch2DPointvertspv_size, spirv_Batch2DPointfragspv.data(), spirv_Batch2DPointfragspv_size)));
            shaderLibrary->AddResource("Batch2DLine", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Batch2DLinevertspv.data(), spirv_Batch2DLinevertspv_size, spirv_Batch2DLinefragspv.data(), spirv_Batch2DLinefragspv_size)));
            shaderLibrary->AddResource("Batch2D", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Batch2Dvertspv.data(), spirv_Batch2Dvertspv_size, spirv_Batch2Dfragspv.data(), spirv_Batch2Dfragspv_size)));
            shaderLibrary->AddResource("FinalPass", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ScreenPassfragspv.data(), spirv_ScreenPassfragspv_size)));
            shaderLibrary->AddResource("Grid", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Gridvertspv.data(), spirv_Gridvertspv_size, spirv_Gridfragspv.data(), spirv_Gridfragspv_size)));
            shaderLibrary->AddResource("CreateEnvironmentMap", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_CreateEnvironmentMapfragspv.data(), spirv_CreateEnvironmentMapfragspv_size)));
            shaderLibrary->AddResource("EnvironmentIrradiance", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_EnvironmentIrradiancefragspv.data(), spirv_EnvironmentIrradiancefragspv_size)));
            shaderLibrary->AddResource("EnvironmentMipFilter", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_EnvironmentMipFilterfragspv.data(), spirv_EnvironmentMipFilterfragspv_size)));
            shaderLibrary->AddResource("FXAA", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_FXAAfragspv.data(), spirv_FXAAfragspv_size)));

            //TODO: Create Compute Shader
//            if(Renderer::capabilities.SupportCompute)
//                shaderLibrary->AddResource("FXAAComp", SharedPtr<Shader>(Shader::CreateCompFromEmbeddedArray(spirv_FXAAComputecompspv.data(), spirv_FXAAComputecompspv_size)));
            shaderLibrary->AddResource("FilmicGrain", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_FilmicGrainfragspv.data(), spirv_FilmicGrainfragspv_size)));
            //                shaderLibrary->AddResource("Outline", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Outlinefragspv.data(), spirv_Outlinefragspv_size)));
            shaderLibrary->AddResource("Debanding", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Debandingfragspv.data(), spirv_Debandingfragspv_size)));
            shaderLibrary->AddResource("ChromaticAberation", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ChromaticAberationfragspv.data(), spirv_ChromaticAberationfragspv_size)));
            shaderLibrary->AddResource("DepthPrePass", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_DepthPrePassfragspv.data(), spirv_DepthPrePassfragspv_size)));
            shaderLibrary->AddResource("ToneMapping", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ToneMappingfragspv.data(), spirv_ToneMappingfragspv_size)));
            shaderLibrary->AddResource("Bloom", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Bloomfragspv.data(), spirv_Bloomfragspv_size)));
//            if(Renderer::capabilities.SupportCompute)
//                shaderLibrary->AddResource("BloomComp", SharedPtr<Shader>(Shader::CreateCompFromEmbeddedArray(spirv_Bloomcompspv.data(), spirv_Bloomcompspv_size)));
            shaderLibrary->AddResource("BRDFLUT", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_BRDFLUTfragspv.data(), spirv_BRDFLUTfragspv_size)));
            shaderLibrary->AddResource("Text", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_Textvertspv.data(), spirv_Textvertspv_size, spirv_Textfragspv.data(), spirv_Textfragspv_size)));
            shaderLibrary->AddResource("DepthOfField", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_DepthOfFieldfragspv.data(), spirv_DepthOfFieldfragspv_size)));
            shaderLibrary->AddResource("Sharpen", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Sharpenfragspv.data(), spirv_Sharpenfragspv_size)));
            shaderLibrary->AddResource("SSAO", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_SSAOfragspv.data(), spirv_SSAOfragspv_size)));
            shaderLibrary->AddResource("SSAOBlur", SharedPtr<Shader>(rhiFactory->CreateShaderFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_SSAOBlurfragspv.data(), spirv_SSAOBlurfragspv_size)));

        }
        else
        {
            //TODO: need check
            LOG("Loading Shaders...");
            String releasePath = "//Shaders";
            shaderLibrary->AddResource("Skybox", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Skybox.shader")));
            shaderLibrary->AddResource("ForwardPBR", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/ForwardPBR.shader")));
            shaderLibrary->AddResource("Shadow", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Shadow.shader")));
            shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Batch2DPoint.shader")));
            shaderLibrary->AddResource("Batch2DLine", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Batch2DLine.shader")));
            shaderLibrary->AddResource("Batch2D", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Batch2D.shader")));
            shaderLibrary->AddResource("FinalPass", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/ScreenPass.shader")));
            shaderLibrary->AddResource("Grid", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Grid.shader")));
            shaderLibrary->AddResource("CreateEnvironmentMap", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/CreateEnvironmentMap.shader")));
            shaderLibrary->AddResource("EnvironmentIrradiance", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/EnvironmentIrradiance.shader")));
            shaderLibrary->AddResource("EnvironmentMipFilter", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/EnvironmentMipFilter.shader")));

            shaderLibrary->AddResource("FXAA", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/FXAA.shader")));

            if(Renderer::capabilities.SupportCompute)
                shaderLibrary->AddResource("FXAAComp", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/FXAACompute.shader")));

            shaderLibrary->AddResource("Debanding", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Debanding.shader")));
            shaderLibrary->AddResource("FilmicGrain", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/FilmicGrain.shader")));
//             shaderLibrary->AddResource("Outline", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Outline.shader")));
            shaderLibrary->AddResource("ChromaticAberation", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/ChromaticAberation.shader")));
            shaderLibrary->AddResource("DepthPrePass", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/DepthPrePass.shader")));
            shaderLibrary->AddResource("ToneMapping", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/ToneMapping.shader")));
            shaderLibrary->AddResource("Bloom", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Bloom.shader")));
            if(Renderer::capabilities.SupportCompute)
                shaderLibrary->AddResource("BloomComp", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/BloomComp.shader")));
            shaderLibrary->AddResource("DepthOfField", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/DepthOfField.shader")));

            shaderLibrary->AddResource("BRDFLUT", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/BRDFLUT.shader")));
            shaderLibrary->AddResource("Text", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Text.shader")));
            shaderLibrary->AddResource("SSAO", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/SSAO.shader")));
            shaderLibrary->AddResource("SSAOBlur", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/SSAOBlur.shader")));
            shaderLibrary->AddResource("Sharpen", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Sharpen.shader")));
        }
    }

    void NekoEngine::Renderer::DrawMesh(NekoEngine::CommandBuffer* commandBuffer, NekoEngine::Pipeline* pipeline,
                                        NekoEngine::Mesh* mesh)
    {
        mesh->GetVertexBuffer()->Bind(commandBuffer, pipeline);
        mesh->GetIndexBuffer()->Bind(commandBuffer);
        DrawIndexed(commandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());
        mesh->GetVertexBuffer()->Unbind();
        mesh->GetIndexBuffer()->Unbind();
    }

}
