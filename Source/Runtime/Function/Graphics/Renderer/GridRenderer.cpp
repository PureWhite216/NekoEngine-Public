#include "GridRenderer.h"
#include "Engine.h"
#include "Renderable/MeshFactory.h"
namespace NekoEngine
{
    GridRenderer::GridRenderer(uint32_t width, uint32_t height)
    {
        m_Pipeline = nullptr;

        IRenderer::SetScreenBufferSize(width, height);
        GridRenderer::Init();

        m_GridRes  = 1.4f;
        m_GridSize = 500.0f;
    }

    GridRenderer::~GridRenderer()
    {
        delete m_Quad;
    }

    void GridRenderer::RenderScene()
    {
        m_CurrentBufferID = 0;
        if(!m_RenderTexture)
            m_CurrentBufferID = GET_SWAP_CHAIN()->GetCurrentBufferIndex();

        CreateGraphicsPipeline();
        auto commandBuffer = GET_SWAP_CHAIN()->GetCurrentCommandBuffer();

        m_Pipeline->Bind(commandBuffer);

        m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

        m_Quad->GetVertexBuffer()->Bind(commandBuffer, m_Pipeline.get());
        m_Quad->GetIndexBuffer()->Bind(commandBuffer);
        
        auto renderer = gEngine->GetRenderer();
        renderer->BindDescriptorSets(m_Pipeline.get(), commandBuffer, 0, m_CurrentDescriptorSets.data(), 1);
        renderer->DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_Quad->GetIndexBuffer()->GetCount());

        m_Quad->GetVertexBuffer()->Unbind();
        m_Quad->GetIndexBuffer()->Unbind();

        End();

        m_Pipeline->End(commandBuffer);

        // if(!m_RenderTexture)
        // Renderer::Present((m_CommandBuffers[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()].get()));
    }

    enum VSSystemUniformIndices : int32_t
    {
        VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
        VSSystemUniformIndex_Size
    };

    void GridRenderer::Init()
    {
        m_Shader = GET_SHADER_LIB()->GetResource("Grid");
        m_Quad   = MeshFactory::CreateQuad();

        DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Shader.get();
        m_DescriptorSet.resize(1);
        m_DescriptorSet[0] = SharedPtr<DescriptorSet>(GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc));
        UpdateUniformBuffer();

        m_CurrentDescriptorSets.resize(1);
    }

    void GridRenderer::Begin()
    {
    }

    void GridRenderer::BeginScene(Level* level, Camera* overrideCamera, Transform* overrideCameraTransform)
    {
        auto& registry = level->GetRegistry();

        if(overrideCamera)
        {
            m_Camera          = overrideCamera;
            m_CameraTransform = overrideCameraTransform;
        }
        else
        {
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                m_Camera          = &cameraView.get<Camera>(cameraView.front());
                m_CameraTransform = registry.try_get<Transform>(cameraView.front());
            }
        }

        if(!m_Camera || !m_CameraTransform)
            return;

        auto proj = m_Camera->GetProjectionMatrix();
        auto view = glm::inverse(m_CameraTransform->GetWorldMatrix());

        UBOFrag test;
        test.Near          = m_Camera->GetNear();
        test.Far           = m_Camera->GetFar();
        test.cameraPos     = glm::vec4(m_CameraTransform->GetWorldPosition(), 1.0f);
        test.cameraForward = glm::vec4(m_CameraTransform->GetForwardDirection(), 1.0f);

        test.maxDistance = m_MaxDistance;

        auto invViewProj = proj * view;
        m_DescriptorSet[0]->SetUniform("UBO", "u_MVP", &invViewProj);
        m_DescriptorSet[0]->SetUniform("UBO", "view", &view);
        m_DescriptorSet[0]->SetUniform("UBO", "proj", &proj);

        m_DescriptorSet[0]->SetUniformBufferData("UniformBuffer", &test);
        m_DescriptorSet[0]->Update();
    }

    void GridRenderer::End()
    {
        
    }

    void GridRenderer::OnImGui()
    {
        
        /*ImGui::TextUnformatted("Grid Renderer");

        if(ImGui::TreeNode("Parameters"))
        {
            ImGui::DragFloat("Resolution", &m_GridRes, 1.0f, 0.0f, 10.0f);
            ImGui::DragFloat("Scale", &m_GridSize, 1.0f, 1.0f, 10000.0f);
            ImGui::DragFloat("Max Distance", &m_MaxDistance, 1.0f, 1.0f, 10000.0f);

            ImGui::TreePop();
        }*/
    }

    void GridRenderer::OnResize(uint32_t width, uint32_t height)
    {
        
        SetScreenBufferSize(width, height);

        UpdateUniformBuffer();
    }

    void GridRenderer::CreateGraphicsPipeline()
    {
        

        PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_Shader;

        pipelineDesc.polygonMode         = PolygonMode::FILL;
        pipelineDesc.cullMode            = CullMode::BACK;
        pipelineDesc.isTransparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;

        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_DepthTexture); // reinterpret_cast<Texture*>(gEngine->GetSceneRenderer()->GetDepthTexture());
        }

        if(m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_RenderTexture;
        else
            pipelineDesc.isSwapChainTarget = true;

        m_Pipeline = Pipeline::Get(pipelineDesc);
    }

    void GridRenderer::UpdateUniformBuffer()
    {
    }

    void GridRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
    {
        m_RenderTexture = texture;

        if(!rebuildFramebuffer)
            return;
    }

} // NekoEngine