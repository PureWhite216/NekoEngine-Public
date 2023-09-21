#pragma once
#include "Vk.h"
#include "Core.h"
#include "RHI/GraphicsContext.h"
namespace NekoEngine
{
#define GET_INSTANCE() gVulkanContext.GetVkInstance()
#define GET_DEVICE() gVulkanContext.GetDevice()->GetHandle()
#define GET_GPU_HANDLE() gVulkanContext.GetDevice()->GetPhysicalDevice()->GetHandle()
#define GET_COMMAND_POOL() gVulkanContext.GetDevice()->GetCommandPool()->GetHandle()
#define GET_ALLOCATOR() gVulkanContext.GetDevice()->GetAllocator()

    struct DeletionQueue
    {
        Deque<std::function<void()>> deletors;

        template<typename T>
        void Push(T&& function)
        {
            deletors.emplace_back(std::forward<T>(function));
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
            {
                (*it)();
            }
            deletors.clear();
        }
    };

    class VulkanSwapChain;
    class VulkanDevice;

    class VulkanContext : public GraphicsContext
    {
    private:
        VkInstance vkInstance = nullptr;
        SharedPtr<VulkanDevice> device;

        std::vector<VkLayerProperties> m_InstanceLayers;
        std::vector<VkExtensionProperties> m_InstanceExtensions;

        std::vector<const char*> m_InstanceLayerNames;
        std::vector<const char*> m_InstanceExtensionNames;

        VkDebugReportCallbackEXT m_DebugCallback = VK_NULL_HANDLE;

    protected:
        static const std::vector<const char*> GetRequiredExtensions();
        const std::vector<const char*> GetRequiredLayers() const;

    public:
        VulkanContext() = default;
        ~VulkanContext() override;

        void Init() override;
        void Present() override;
        void WaitIdle() const override;
        void SetupDebugCallback();

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                            VkDebugReportObjectTypeEXT objType,
                                                            uint64_t sourceObj,
                                                            size_t location,
                                                            int32_t msgCode,
                                                            const char* pLayerPrefix,
                                                            const char* pMsg,
                                                            void* userData);

        size_t GetMinUniformBufferOffsetAlignment() const override;

        bool FlipImGUITexture() const override;

        void OnImGui() override;

        VkInstance GetVkInstance() const
        {
            if(vkInstance == nullptr)
                throw std::runtime_error("VkInstance is nullptr");
            return vkInstance;
        }

        SharedPtr<VulkanDevice> GetDevice() const
        {
            if(device == nullptr)
                throw std::runtime_error("VulkanDevice is nullptr");
            return device;
        }

        VulkanSwapChain* GetSwapChain() const;


    protected:
        void CreateInstance();
    };

    extern VulkanContext gVulkanContext;
} // NekoEngine

