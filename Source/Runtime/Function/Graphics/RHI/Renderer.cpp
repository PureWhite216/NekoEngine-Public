#include "Renderer.h"
#include "RHIFactory.h"
namespace NekoEngine
{
    RenderAPICapabilities Renderer::capabilities = RenderAPICapabilities();

    void Renderer::LoadEngineShaders(bool loadEmbeddedShaders)
    {
        LOG("Loading engine Shaders...");
        if(loadEmbeddedShaders)
        {
            //TODO: Load embedded Shaders
        }
        else
        {
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
            // shaderLibrary->AddResource("Outline", SharedPtr<Shader>(rhiFactory->CreateShader("//CoreShaders/Outline.shader")));
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
