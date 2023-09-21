#include "GLFW/glfw3.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Engine.h"
#include "vulkan/vulkan_win32.h"

namespace NekoEngine
{
    VulkanContext gVulkanContext{};

    const std::vector<const char*> VulkanContext::GetRequiredExtensions()
    {
//         unsigned int glfwExtensionCount = 0;
//         const char** glfwExtensions;
//
//         glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//         std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        std::vector<const char*> extensions;

//        if(EnableValidationLayers)
//        {
        // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        // extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        // extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
//        }

        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back("VK_KHR_portability_enumeration");
        extensions.push_back("VK_KHR_win32_surface");
//        extensions.push_back("VK_KHR_win32_surface");
//        extensions.push_back("VK_KHR_surface");
        // extensions.push_back("VK_KHR_DISPLAY");
        return extensions;
    }

    const std::vector<const char*> VulkanContext::GetRequiredLayers() const
    {
        std::vector<const char*> layers = {};

//        if(EnableValidationLayers)
//        {
//            layers.emplace_back(VK_LAYER_LUNARG_VALIDATION_NAME);
//        }

        return layers;
    }

    VulkanContext::~VulkanContext()
    {
        vkDestroyInstance(vkInstance, nullptr);
    }


    void VulkanContext::Init()
    {
        LOG("Initializing Vulkan context");
        CreateInstance();

        device = MakeShared<VulkanDevice>();
        device->Init();
        //TODO: Set Debug CallBAck

        LOG("Vulkan context initialized");
    }

    void VulkanContext::Present()
    {

    }

    void VulkanContext::WaitIdle() const
    {
        vkDeviceWaitIdle(GET_DEVICE());
    }



    void VulkanContext::CreateInstance()
    {
        if(volkInitialize() != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to initialize volk");
        }

        m_InstanceLayerNames     = GetRequiredLayers();
        m_InstanceExtensionNames = GetRequiredExtensions();

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "NekoEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NekoEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_InstanceExtensionNames.size());
        createInfo.ppEnabledExtensionNames = m_InstanceExtensionNames.data();;
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_InstanceLayerNames.size());
        createInfo.ppEnabledLayerNames = m_InstanceLayerNames.data();
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
//        createInfo.enabledExtensionCount = glfwExtensionCount;
//        createInfo.ppEnabledExtensionNames = glfwExtensions;

        LOG("Creating Vulkan instance");
        if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }

        volkLoadInstance(vkInstance);
    }

    size_t VulkanContext::GetMinUniformBufferOffsetAlignment() const
    {
        return gVulkanContext.GetDevice()->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
    }

    bool VulkanContext::FlipImGUITexture() const
    {
        return true;
    }

    void VulkanContext::OnImGui()
    {

    }

    VulkanSwapChain* VulkanContext::GetSwapChain() const
    {
        return dynamic_cast<VulkanSwapChain*>(gEngine->GetSwapChain());
    }
} // NekoEngine