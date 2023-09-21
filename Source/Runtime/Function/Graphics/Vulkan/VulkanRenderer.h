#pragma once
#include "RHI/Renderer.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "Vk.h"
namespace NekoEngine
{
    class VulkanRenderer : public Renderer
    {
    private:
        uint32_t currentSemaphoreIndex = 0;
        String rendererTitle;
        uint32_t descriptorCapacity = 0;
        VkDescriptorSet descriptorSetPool[16] = {};

        static int deletionQueueIndex;
        static VkDescriptorPool descriptorPool;
        static ArrayList<DeletionQueue> deletionQueue;
        static int currentDeletionQueue;

    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() = default;
        void Init(bool loadEmbeddedShaders = false) override;
        void Begin() override;
        void Present() override;
        void ClearRenderTarget(Texture* texture, CommandBuffer* commandBuffer, Color clearColour) override;
        void ClearSwapChainImage() const;
        void OnResize(uint32_t height, uint32_t width);
        void BindDescriptorSets(Pipeline* pipeline, CommandBuffer* commandBuffer, uint32_t dynamicOffset, DescriptorSet** descriptorSets, uint32_t descriptorCount) override;
        void Draw(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType = DataType::UNSIGNED_INT, void* indices = nullptr) override;
        void DrawIndexed(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) override;
        void Dispatch(CommandBuffer* commandBuffer, uint32_t workGroupSizeX, uint32_t workGroupSizeY, uint32_t workGroupSizeZ) override;
        void SaveScreenshot(const std::string& path, Texture* texture = nullptr) override;


//        SwapChain* GetSwapChain() const override
//        {
//            return gVulkanContext.GetSwapChain();
//        }

        GraphicsContext* GetGraphicsContext() const override
        {
            return dynamic_cast<GraphicsContext*>(&gVulkanContext);
        }

        const String &GetRendererTitle() const
        {
            return rendererTitle;
        }

        uint32_t GetGPUCount() const override;

        static VkDescriptorPool GetDescriptorPool();

        static DeletionQueue& GetCurrentDeletionQueue();

        static DeletionQueue& GetDeletionQueue(int index);

    };

} // NekoEngine
