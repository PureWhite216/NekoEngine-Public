#pragma once
#include "Vk.h"
#include "RHI/SwapChain.h"

#define MAX_BUFFER_COUNT 3
namespace NekoEngine
{
    class VulkanCommandBuffer;
    class VulkanCommandPool;

    struct BufferData
    {
        VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
        SharedPtr<VulkanCommandPool> CommandPool;
        SharedPtr<VulkanCommandBuffer> MainCommandBuffer;
    };

    class VulkanSwapChain : public SwapChain
    {
    private:
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        VkSwapchainKHR oldHandle = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkFormat colorFormat = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;

        uint32_t width;
        uint32_t height;
        bool isVSync = false;

        BufferData bufferData[MAX_BUFFER_COUNT];
        ArrayList<Texture2D*> swapChainBuffers;
        uint32_t swapChainBufferNum = 0;

        uint32_t currentBufferIndex = 0;
        uint32_t acquireImageIndex = UINT_MAX;
    public:
        VulkanSwapChain(uint32_t _width, uint32_t _height);
        ~VulkanSwapChain() override;

//        bool Init(bool vsync) override;
        bool Init(bool vsync) override;
        bool Init(bool vsync, Window* windowHandle) override;
        Texture* GetCurrentImage() override;
        Texture* GetImage(uint32_t index) override;
        uint32_t GetCurrentBufferIndex() const override;
        uint32_t GetCurrentImageIndex() const override;
        size_t GetSwapChainBufferCount() const override;
        CommandBuffer* GetCurrentCommandBuffer() override;
        void SetVSync(bool vsync) override;

        const VkSwapchainKHR GetHandle() const
        {
            return handle;
        }
        const VkSurfaceKHR GetSurface() const { return surface; }

        void CreateFrameData();
        void AcquireNextImage();
        void QueueSubmit();
        void Present(VkSemaphore semaphore);
        void Begin();
        void End();
        void OnResize(uint32_t width, uint32_t height, bool forceResize = false, Window* windowHandle = nullptr);
        VkSurfaceKHR CreateSurface(VkInstance vkInstance, Window* window);
        void InitImageFormatAndColourSpace();

        BufferData& GetCurrentBufferData() { return bufferData[currentBufferIndex]; }
    };

} // NekoEngine
