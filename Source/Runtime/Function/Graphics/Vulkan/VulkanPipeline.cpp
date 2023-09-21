#include "VulkanPipeline.h"
#include "VulkanRenderer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanFrameBuffer.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanUtility.h"


namespace NekoEngine
{
    VulkanPipeline::VulkanPipeline(const PipelineDesc &pipelineDesc)
    {
        Init(pipelineDesc);
    }

    bool VulkanPipeline::Init(const PipelineDesc &pipelineDesc)
    {
        this->pipelineDesc = pipelineDesc;
        shader = pipelineDesc.shader;
        layout = dynamic_cast<VulkanShader*>(shader.get())->GetPipelineLayout();

        TransitionAttachments();

        // Pipeline
        std::vector<VkDynamicState> dynamicStateDescriptors;
        VkPipelineDynamicStateCreateInfo dynamicStateCI {};
        dynamicStateCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pNext          = NULL;
        dynamicStateCI.pDynamicStates = dynamicStateDescriptors.data();

        std::vector<VkVertexInputAttributeDescription> vertexInputDescription;

        isCompute = dynamic_cast<VulkanShader*>(shader.get())->IsCompute();

        if(isCompute)
        {
            VkComputePipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.layout                      = layout;
            pipelineInfo.stage                       = dynamic_cast<VulkanShader*>(shader.get())->GetShaderStages()[0];
            VK_CHECK_RESULT(vkCreateComputePipelines(GET_DEVICE(), nullptr, 1, &pipelineInfo, nullptr, &handle), "failed to create compute pipeline!");
        }
        else
        {
            CreateFramebuffers();

            uint32_t stride = dynamic_cast<VulkanShader*>(shader.get())->GetVertexInputStride();

            // Vertex layout
            VkVertexInputBindingDescription vertexBindingDescription;

            if(stride > 0)
            {
                vertexBindingDescription.binding   = 0;
                vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexBindingDescription.stride    = dynamic_cast<VulkanShader*>(shader.get())->GetVertexInputStride();
            }

            const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription = dynamic_cast<VulkanShader*>(shader.get())->GetVertexInputAttributeDescription();

            VkPipelineVertexInputStateCreateInfo vi {};
            vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vi.pNext                           = NULL;
            vi.vertexBindingDescriptionCount   = stride > 0 ? 1 : 0;
            vi.pVertexBindingDescriptions      = stride > 0 ? &vertexBindingDescription : nullptr;
            vi.vertexAttributeDescriptionCount = stride > 0 ? uint32_t(vertexInputAttributeDescription.size()) : 0;
            vi.pVertexAttributeDescriptions    = stride > 0 ? vertexInputAttributeDescription.data() : nullptr;

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI {};
            inputAssemblyCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyCI.pNext                  = NULL;
            inputAssemblyCI.primitiveRestartEnable = VK_FALSE;
            inputAssemblyCI.topology               = VulkanUtility::DrawTypeToVk(pipelineDesc.drawType);

            VkPipelineRasterizationStateCreateInfo rs {};
            rs.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rs.polygonMode             = VulkanUtility::PolygonModeToVk(pipelineDesc.polygonMode);
            rs.cullMode                = VulkanUtility::CullModeToVK(pipelineDesc.cullMode);
            rs.frontFace               = pipelineDesc.isSwapChainTarget ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
            rs.depthClampEnable        = VK_FALSE;
            rs.rasterizerDiscardEnable = VK_FALSE;
            rs.depthBiasEnable         = (pipelineDesc.isDepthBiasEnabled ? VK_TRUE : VK_FALSE);
            rs.depthBiasConstantFactor = pipelineDesc.depthBiasConstantFactor;
            rs.depthBiasClamp          = 0;
            rs.depthBiasSlopeFactor    = pipelineDesc.depthBiasSlopeFactor;

            if(Renderer::capabilities.WideLines)
                rs.lineWidth = pipelineDesc.lineWidth;
            else
                rs.lineWidth = 1.0f;
            rs.pNext = NULL;

            VkPipelineColorBlendStateCreateInfo cb {};
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.pNext = NULL;
            cb.flags = 0;

            std::vector<VkPipelineColorBlendAttachmentState> blendAttachState;
            blendAttachState.resize(dynamic_cast<VulkanRenderPass*>(renderPass.get())->GetColourAttachmentCount());

            for(unsigned int i = 0; i < blendAttachState.size(); i++)
            {
                blendAttachState[i]                = VkPipelineColorBlendAttachmentState();
                blendAttachState[i].colorWriteMask = 0x0f;
                blendAttachState[i].alphaBlendOp   = VK_BLEND_OP_ADD;
                blendAttachState[i].colorBlendOp   = VK_BLEND_OP_ADD;

                if(pipelineDesc.isTransparencyEnabled)
                {
                    blendAttachState[i].blendEnable         = VK_TRUE;
                    blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

                    if(pipelineDesc.blendMode == BlendMode::SrcAlphaOneMinusSrcAlpha)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    }
                    else if(pipelineDesc.blendMode == BlendMode::ZeroSrcColor)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
                    }
                    else if(pipelineDesc.blendMode == BlendMode::OneZero)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    }
                    else
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    }
                }
                else
                {
                    blendAttachState[i].blendEnable         = VK_FALSE;
                    blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                }
            }

            cb.attachmentCount   = static_cast<uint32_t>(blendAttachState.size());
            cb.pAttachments      = blendAttachState.data();
            cb.logicOpEnable     = VK_FALSE;
            cb.logicOp           = VK_LOGIC_OP_NO_OP;
            cb.blendConstants[0] = 1.0f;
            cb.blendConstants[1] = 1.0f;
            cb.blendConstants[2] = 1.0f;
            cb.blendConstants[3] = 1.0f;

            VkPipelineViewportStateCreateInfo vp {};
            vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vp.pNext         = NULL;
            vp.viewportCount = 1;
            vp.scissorCount  = 1;
            vp.pScissors     = NULL;
            vp.pViewports    = NULL;
            dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_SCISSOR);

            if(Renderer::capabilities.WideLines && pipelineDesc.polygonMode == PolygonMode::LINE) // || pipelineDesc.polygonMode == PolygonMode::LINESTRIP)
                dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

            if(pipelineDesc.isDepthBiasEnabled)
            {
                dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
                depthBiasConstantFactor = pipelineDesc.depthBiasConstantFactor;
                depthBiasSlopeFactor    = pipelineDesc.depthBiasSlopeFactor;
                isDepthBiasEnabled  = true;
            }
            else
            {
                isDepthBiasEnabled = false;
            }

            VkPipelineDepthStencilStateCreateInfo ds {};
            ds.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            ds.pNext                 = NULL;
            ds.depthTestEnable       = VK_TRUE;
            ds.depthWriteEnable      = VK_TRUE;
            ds.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
            ds.depthBoundsTestEnable = VK_FALSE;
            ds.stencilTestEnable     = VK_FALSE;
            ds.back.failOp           = VK_STENCIL_OP_KEEP;
            ds.back.passOp           = VK_STENCIL_OP_KEEP;
            ds.back.compareOp        = VK_COMPARE_OP_ALWAYS;
            ds.back.compareMask      = 0;
            ds.back.reference        = 0;
            ds.back.depthFailOp      = VK_STENCIL_OP_KEEP;
            ds.back.writeMask        = 0;
            ds.minDepthBounds        = 0;
            ds.maxDepthBounds        = 0;
            ds.front                 = ds.back;

            VkPipelineMultisampleStateCreateInfo ms {};
            ms.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            ms.pNext                 = NULL;
            ms.pSampleMask           = NULL;
            ms.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
            ms.sampleShadingEnable   = VK_FALSE;
            ms.alphaToCoverageEnable = VK_FALSE;
            ms.alphaToOneEnable      = VK_FALSE;
            ms.minSampleShading      = 0.0;

            dynamicStateCI.dynamicStateCount = uint32_t(dynamicStateDescriptors.size());
            dynamicStateCI.pDynamicStates    = dynamicStateDescriptors.data();

            auto vkshader = dynamic_cast<VulkanShader*>(pipelineDesc.shader.get());
            VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
            graphicsPipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphicsPipelineCreateInfo.pNext               = NULL;
            graphicsPipelineCreateInfo.layout              = layout;
            graphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
            graphicsPipelineCreateInfo.basePipelineIndex   = -1;
            graphicsPipelineCreateInfo.pVertexInputState   = &vi;
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCI;
            graphicsPipelineCreateInfo.pRasterizationState = &rs;
            graphicsPipelineCreateInfo.pColorBlendState    = &cb;
            graphicsPipelineCreateInfo.pTessellationState  = VK_NULL_HANDLE;
            graphicsPipelineCreateInfo.pMultisampleState   = &ms;
            graphicsPipelineCreateInfo.pDynamicState       = &dynamicStateCI;
            graphicsPipelineCreateInfo.pViewportState      = &vp;
            graphicsPipelineCreateInfo.pDepthStencilState  = &ds;
            graphicsPipelineCreateInfo.pStages             = vkshader->GetShaderStages();
            graphicsPipelineCreateInfo.stageCount          = vkshader->GetStageCount();
            graphicsPipelineCreateInfo.renderPass          = dynamic_cast<VulkanRenderPass*>(renderPass.get())->GetHandle();
            graphicsPipelineCreateInfo.subpass             = 0;

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(GET_DEVICE(),gVulkanContext.GetDevice()->GetPipelineCache(), 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &handle), "failed to create graphics pipeline!");
        }

        if(!pipelineDesc.DebugName.empty())
            VulkanUtility::SetDebugUtilsObjectName(GET_DEVICE(), VK_OBJECT_TYPE_PIPELINE, pipelineDesc.DebugName, handle);

        return true;
    }

    void VulkanPipeline::TransitionAttachments()
    {
        auto commandBuffer = gVulkanContext.GetSwapChain()->GetCurrentCommandBuffer();
        if(pipelineDesc.isSwapChainTarget)
        {
            for(uint32_t i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
            {
                dynamic_cast<VulkanTexture2D*>(gVulkanContext.GetSwapChain()->GetImage(i))->TransitionImage(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, (VulkanCommandBuffer*)commandBuffer);
            }
        }
        if(pipelineDesc.depthArrayTarget)
        {
            dynamic_cast<VulkanTextureDepthArray*>(pipelineDesc.depthArrayTarget)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, (VulkanCommandBuffer*)commandBuffer);
        }

        if(pipelineDesc.depthTarget)
        {
            dynamic_cast<VulkanTextureDepth*>(pipelineDesc.depthTarget)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, (VulkanCommandBuffer*)commandBuffer);
        }

        for(auto texture : pipelineDesc.colourTargets)
        {
            if(texture != nullptr)
            {
                dynamic_cast<VulkanTexture2D*>(texture)->TransitionImage(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, (VulkanCommandBuffer*)commandBuffer);
            }
        }
    }



    void VulkanPipeline::Bind(CommandBuffer* commandBuffer, uint32_t layer)
    {
        VulkanFramebuffer* framebuffer;

        if(!isCompute)
        {
            TransitionAttachments();
            if(pipelineDesc.isSwapChainTarget)
            {
                framebuffer = frameBuffers[gVulkanContext.GetSwapChain()->GetCurrentImageIndex()].get();
            }
            else if(pipelineDesc.depthArrayTarget || pipelineDesc.cubeMapTarget)
            {
                framebuffer = frameBuffers[layer].get();
            }
            else
            {
                framebuffer = frameBuffers[0].get();
            }

            renderPass->BeginRenderpass(commandBuffer, pipelineDesc.clearColor, framebuffer, SubPassContents::INLINE, GetWidth(), GetHeight());
        }
        else
        {
            //TODO: correct width and height
            commandBuffer->UpdateViewport(1600, 900, false);
        }

        vkCmdBindPipeline(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), isCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, handle);
        if(isDepthBiasEnabled)
        {
            vkCmdSetDepthBias(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), depthBiasConstantFactor, 0.0f, depthBiasSlopeFactor);
        }
    }

    void VulkanPipeline::ClearRenderTargets(CommandBuffer* commandBuffer)
    {
        if(pipelineDesc.isSwapChainTarget)
        {
            for(int i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
            {
                gVulkanContext.GetRenderer()->ClearRenderTarget(gVulkanContext.GetSwapChain()->GetImage(i), commandBuffer, pipelineDesc.clearColor);
            }
        }

        if(pipelineDesc.depthArrayTarget)
        {
            gVulkanContext.GetRenderer()->ClearRenderTarget(pipelineDesc.depthArrayTarget, commandBuffer, pipelineDesc.clearColor);
        }

        if(pipelineDesc.depthTarget)
        {
            gVulkanContext.GetRenderer()->ClearRenderTarget(pipelineDesc.depthTarget, commandBuffer, pipelineDesc.clearColor);
        }

        for(auto texture : pipelineDesc.colourTargets)
        {
            if(texture != nullptr)
            {
                gVulkanContext.GetRenderer()->ClearRenderTarget(texture, commandBuffer, pipelineDesc.clearColor);
            }
        }
    }

    void VulkanPipeline::CreateFramebuffers()
    {
        std::vector<TextureType> attachmentTypes;
        std::vector<Texture*> attachments;

        if(pipelineDesc.isSwapChainTarget)
        {
            attachmentTypes.push_back(TextureType::COLOUR);
            attachments.push_back(gVulkanContext.GetSwapChain()->GetImage(0));
        }
        else
        {
            for(auto texture : pipelineDesc.colourTargets)
            {
                if(texture)
                {
                    attachmentTypes.push_back(texture->GetType());
                    attachments.push_back(texture);
                }
            }
        }

        if(pipelineDesc.depthTarget)
        {
            attachmentTypes.push_back(pipelineDesc.depthTarget->GetType());
            attachments.push_back(pipelineDesc.depthTarget);
        }

        if(pipelineDesc.depthArrayTarget)
        {
            attachmentTypes.push_back(pipelineDesc.depthArrayTarget->GetType());
            attachments.push_back(pipelineDesc.depthArrayTarget);
        }

        if(pipelineDesc.cubeMapTarget)
        {
            attachmentTypes.push_back(pipelineDesc.cubeMapTarget->GetType());
            attachments.push_back(pipelineDesc.cubeMapTarget);
        }

        RenderPassDesc renderPassDesc = {};
        renderPassDesc.attachmentCount          = uint32_t(attachmentTypes.size());
        renderPassDesc.attachmentTypes          = attachmentTypes.data();
        renderPassDesc.attachments              = attachments.data();
        renderPassDesc.swapchainTarget          = pipelineDesc.isSwapChainTarget;
        renderPassDesc.clear                    = pipelineDesc.isClearTargets;
        renderPassDesc.cubeMapIndex             = pipelineDesc.cubeMapIndex;
        renderPassDesc.mipIndex                 = pipelineDesc.mipIndex;
//        renderPassDesc.DebugName                = pipelineDesc.DebugName;

        renderPass = RenderPass::Get(renderPassDesc);

        FramebufferDesc frameBufferDesc = {};
        frameBufferDesc.width           = GetWidth();
        frameBufferDesc.height          = GetHeight();
        frameBufferDesc.attachmentCount = uint32_t(attachments.size());
        frameBufferDesc.renderPass      = renderPass.get();
        frameBufferDesc.attachmentTypes = attachmentTypes.data();

        if(pipelineDesc.isSwapChainTarget)
        {
            for(uint32_t i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
            {
                attachments[0]              = gVulkanContext.GetSwapChain()->GetImage(i);
                frameBufferDesc.screenFBO   = true;
                frameBufferDesc.attachments = attachments.data();

                frameBuffers.emplace_back(new VulkanFramebuffer(frameBufferDesc));
            }
        }
        else if(pipelineDesc.depthArrayTarget)
        {
            for(uint32_t i = 0; i < dynamic_cast<VulkanTextureDepthArray*>(pipelineDesc.depthArrayTarget)->GetCount(); ++i)
            {
                attachments[0]              = pipelineDesc.depthArrayTarget;
                frameBufferDesc.layer       = i;
                frameBufferDesc.screenFBO   = false;
                frameBufferDesc.attachments = attachments.data();

                frameBuffers.emplace_back(new VulkanFramebuffer(frameBufferDesc));
            }
        }
        else if(pipelineDesc.cubeMapTarget)
        {
            for(uint32_t i = 0; i < 6; ++i)
            {
                frameBufferDesc.layer     = i;
                frameBufferDesc.screenFBO = false;
                frameBufferDesc.mipIndex  = pipelineDesc.mipIndex;

                attachments[0]              = pipelineDesc.cubeMapTarget;
                frameBufferDesc.attachments = attachments.data();

                frameBuffers.emplace_back(new VulkanFramebuffer(frameBufferDesc));
            }
        }
        else
        {
            frameBufferDesc.attachments = attachments.data();
            frameBufferDesc.screenFBO   = false;
            frameBufferDesc.mipIndex    = pipelineDesc.mipIndex;
            frameBuffers.emplace_back(new VulkanFramebuffer(frameBufferDesc));
        }
    }

    void VulkanPipeline::End(CommandBuffer* commandBuffer)
    {
        if(!isCompute)
        {
            renderPass->EndRenderpass(commandBuffer);
        }
    }

    VulkanPipeline::~VulkanPipeline()
    {
        DeletionQueue deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();
        deletionQueue.Push([=]() {
            vkDestroyPipeline(GET_DEVICE(), handle, nullptr);
            vkDestroyPipelineLayout(GET_DEVICE(), layout, nullptr);
        });
    }
} // NekoEngine