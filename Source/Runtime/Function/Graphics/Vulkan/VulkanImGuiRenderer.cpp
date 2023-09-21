#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#define VK_NO_PROTOTYPES

#include "VulkanImGuiRenderer.h"
#include "VulkanRenderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "VulkanDevice.h"


static ImGui_ImplVulkanH_Window g_WindowData;
static VkAllocationCallbacks* g_Allocator = nullptr;
static VkDescriptorPool g_DescriptorPool  = VK_NULL_HANDLE;


static void check_vk_result(VkResult err)
{
    if(err == 0)
        return;
    printf("VkResult %d\n", err);
    if(err < 0)
        abort();
}

namespace NekoEngine
{
    VulkanImGuiRenderer::VulkanImGuiRenderer(uint32_t width, uint32_t height, bool clearScreen)
    {
        m_Width = width;
        m_Height = height;
        m_ClearScreen = clearScreen;
    }

    VulkanImGuiRenderer::~VulkanImGuiRenderer()
    {
        for(int i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
        {
            delete m_Framebuffers[i];
        }

        delete m_Renderpass;
        delete m_FontTexture;

        DeletionQueue& deletionQueue = VulkanRenderer::GetCurrentDeletionQueue();

        for(int i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
        {
            ImGui_ImplVulkanH_Frame* fd = &g_WindowData.Frames[i];
            auto fence                  = fd->Fence;
            auto alloc                  = g_Allocator;
            auto commandPool            = fd->CommandPool;

            deletionQueue.Push([fence, commandPool, alloc]
                                       {
                                           vkDestroyFence(GET_DEVICE(), fence, alloc);
                                           vkDestroyCommandPool(GET_DEVICE(), commandPool, alloc); });
        }
        auto descriptorPool = g_DescriptorPool;

        deletionQueue.Push([descriptorPool]
                                   {
                                       vkDestroyDescriptorPool(GET_DEVICE(), descriptorPool, nullptr);
                                       ImGui_ImplVulkan_Shutdown(); });
    }

    void VulkanImGuiRenderer::SetupVulkanWindowData(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width,
                                                    int height)
    {
        {
            VkDescriptorPoolSize pool_sizes[] = {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes                 = pool_sizes;
            VkResult err                         = vkCreateDescriptorPool(GET_DEVICE(), &pool_info, g_Allocator, &g_DescriptorPool);
            check_vk_result(err);
        }

        wd->Surface     = surface;
        wd->ClearEnable = m_ClearScreen;

        auto swapChain = gVulkanContext.GetSwapChain();
        wd->Swapchain  = swapChain->GetHandle();
        wd->Width      = width;
        wd->Height     = height;

        wd->ImageCount = static_cast<uint32_t>(swapChain->GetSwapChainBufferCount());

        TextureType textureTypes[1] = { TextureType::COLOUR };

        Texture* textures[1] = { swapChain->GetImage(0) };

        RenderPassDesc renderPassDesc;
        renderPassDesc.attachmentCount = 1;
        renderPassDesc.attachmentTypes = textureTypes;
        renderPassDesc.clear           = m_ClearScreen;
        renderPassDesc.attachments     = textures;
        renderPassDesc.swapchainTarget = true;

        m_Renderpass   = new VulkanRenderPass(renderPassDesc);
        wd->RenderPass = m_Renderpass->GetHandle();

        wd->Frames = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
        // wd->FrameSemaphores = (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * wd->ImageCount);
        memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);
        // memset(wd->FrameSemaphores, 0, sizeof(wd->FrameSemaphores[0]) * wd->ImageCount);

        // Create The Image Views
        {
            for(uint32_t i = 0; i < wd->ImageCount; i++)
            {
                auto scBuffer                = dynamic_cast<VulkanTexture2D*>(swapChain->GetImage(i));
                wd->Frames[i].Backbuffer     = scBuffer->GetImage();
                wd->Frames[i].BackbufferView = scBuffer->GetImageView();
            }
        }

        TextureType attachmentTypes[1];
        attachmentTypes[0] = TextureType::COLOUR;

        Texture* attachments[1];
        FramebufferDesc frameBufferDesc {};
        frameBufferDesc.width           = wd->Width;
        frameBufferDesc.height          = wd->Height;
        frameBufferDesc.attachmentCount = 1;
        frameBufferDesc.renderPass      = m_Renderpass;
        frameBufferDesc.attachmentTypes = attachmentTypes;
        frameBufferDesc.screenFBO       = true;

        for(uint32_t i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
        {
            attachments[0]              = gVulkanContext.GetSwapChain()->GetImage(i);
            frameBufferDesc.attachments = attachments;

            m_Framebuffers[i]         = new VulkanFramebuffer(frameBufferDesc);
            wd->Frames[i].Framebuffer = m_Framebuffers[i]->GetHandle();
        }
    }

    void VulkanImGuiRenderer::Init()
    {
        LOG("Initializing Vulkan ImGui Renderer");
        int w, h;
        w                            = (int)m_Width;
        h                            = (int)m_Height;
        ImGui_ImplVulkanH_Window* wd = &g_WindowData;
        VkSurfaceKHR surface         = gVulkanContext.GetSwapChain()->GetSurface();
        SetupVulkanWindowData(wd, surface, w, h);

        // Setup Vulkan binding
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = GET_INSTANCE();
        init_info.PhysicalDevice            = GET_GPU_HANDLE();
        init_info.Device                    = GET_DEVICE();
        init_info.QueueFamily               = gVulkanContext.GetDevice()->GetPhysicalDevice()->GetGraphicsFamilyIndex();
        init_info.Queue                     = gVulkanContext.GetDevice()->GetGraphicsQueue();
        init_info.PipelineCache             = gVulkanContext.GetDevice()->GetPipelineCache();
        init_info.DescriptorPool            = g_DescriptorPool;
        init_info.Allocator                 = g_Allocator;
        init_info.CheckVkResultFn           = NULL;
        init_info.MinImageCount             = 2;
        init_info.ImageCount                = (uint32_t)gVulkanContext.GetSwapChain()->GetSwapChainBufferCount();
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
        // Upload Fonts
        {
            ImGuiIO& io = ImGui::GetIO();

            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            m_FontTexture   = new VulkanTexture2D(width, height, pixels, TextureDesc(TextureFilter::NEAREST, TextureFilter::NEAREST));
            io.Fonts->TexID = (ImTextureID)m_FontTexture->GetHandle();
        }
    }

    void VulkanImGuiRenderer::FrameRender(ImGui_ImplVulkanH_Window* wd)
    {
        wd->FrameIndex            = gVulkanContext.GetSwapChain()->GetCurrentImageIndex();
        auto currentCommandBuffer = (VulkanCommandBuffer*)gVulkanContext.GetSwapChain()->GetCurrentCommandBuffer();
        auto& descriptorImageMap  = ImGui_ImplVulkan_GetDescriptorImageMap();
        auto swapChain            = gVulkanContext.GetSwapChain();

        if(wd->Swapchain != swapChain->GetHandle())
            OnResize(wd->Width, wd->Height);

        {
            auto draw_data = ImGui::GetDrawData();
            for(int n = 0; n < draw_data->CmdListsCount; n++)
            {
                const ImDrawList* cmd_list = draw_data->CmdLists[n];
                for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                    const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

                    if((Texture*)pcmd->TextureId)
                    {
                        if(((Texture*)pcmd->TextureId)->GetType() == TextureType::COLOUR)
                        {
                            auto texture = (VulkanTexture2D*)pcmd->TextureId;
                            texture->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, currentCommandBuffer);
                            descriptorImageMap[pcmd->TextureId] = texture->GetDescriptor();
                        }
                        else if(((Texture*)pcmd->TextureId)->GetType() == TextureType::DEPTH)
                        {
                            auto texture = (VulkanTextureDepth*)pcmd->TextureId;
                            texture->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, currentCommandBuffer);
                            descriptorImageMap[pcmd->TextureId] = texture->GetDescriptor();
                        }
                        else if(((Texture*)pcmd->TextureId)->GetType() == TextureType::DEPTHARRAY)
                        {
                            auto texture = (VulkanTextureDepthArray*)pcmd->TextureId;
                            texture->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, currentCommandBuffer);
                            descriptorImageMap[pcmd->TextureId] = texture->GetDescriptor();
                        }
                    }
                }
            }
        }

        ImGui_ImplVulkan_CreateDescriptorSets(ImGui::GetDrawData(), wd->FrameIndex);

        Color clearColour = { 0.1f, 0.1f, 0.1f, 1.0f };
        m_Renderpass->BeginRenderpass(currentCommandBuffer, clearColour, m_Framebuffers[wd->FrameIndex], SubPassContents::INLINE, wd->Width, wd->Height);

        {
            // Record Imgui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), currentCommandBuffer->GetHandle(), VK_NULL_HANDLE, wd->FrameIndex);
        }
        m_Renderpass->EndRenderpass(currentCommandBuffer);
    }

    void VulkanImGuiRenderer::Render(CommandBuffer* commandBuffer)
    {
        ImGui::Render();
        FrameRender(&g_WindowData);
    }

    void VulkanImGuiRenderer::OnResize(uint32_t width, uint32_t height)
    {
        auto* wd       = &g_WindowData;
        auto swapChain = gVulkanContext.GetSwapChain();
        wd->Swapchain  = swapChain->GetHandle();
        for(uint32_t i = 0; i < wd->ImageCount; i++)
        {
            auto scBuffer                = dynamic_cast<VulkanTexture2D*>(swapChain->GetImage(i));
            wd->Frames[i].Backbuffer     = scBuffer->GetImage();
            wd->Frames[i].BackbufferView = scBuffer->GetImageView();
        }

        wd->Width  = width;
        wd->Height = height;

        for(uint32_t i = 0; i < wd->ImageCount; i++)
        {
            delete m_Framebuffers[i];
        }
        // Create Framebuffer
        TextureType attachmentTypes[1];
        attachmentTypes[0] = TextureType::COLOUR;

        Texture* attachments[1];
        FramebufferDesc frameBufferDesc {};
        frameBufferDesc.width           = wd->Width;
        frameBufferDesc.height          = wd->Height;
        frameBufferDesc.attachmentCount = 1;
        frameBufferDesc.renderPass      = m_Renderpass;
        frameBufferDesc.attachmentTypes = attachmentTypes;
        frameBufferDesc.screenFBO       = true;

        for(uint32_t i = 0; i < gVulkanContext.GetSwapChain()->GetSwapChainBufferCount(); i++)
        {
            attachments[0]              = gVulkanContext.GetSwapChain()->GetImage(i);
            frameBufferDesc.attachments = attachments;

            m_Framebuffers[i]         = new VulkanFramebuffer(frameBufferDesc);
            wd->Frames[i].Framebuffer = m_Framebuffers[i]->GetHandle();
        }
    }

    void VulkanImGuiRenderer::Clear()
    {
        
    }

    void VulkanImGuiRenderer::RebuildFontTexture()
    {
        ImGuiIO& io = ImGui::GetIO();

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        m_FontTexture   = new VulkanTexture2D(width, height, pixels, TextureDesc(TextureFilter::NEAREST, TextureFilter::NEAREST, TextureWrap::REPEAT));
        io.Fonts->TexID = (ImTextureID)m_FontTexture->GetHandle();
    }

} // NekoEngine