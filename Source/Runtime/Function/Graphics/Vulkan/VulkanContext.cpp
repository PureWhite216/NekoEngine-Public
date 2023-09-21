#include "GLFW/glfw3.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Engine.h"
#include "vulkan/vulkan_win32.h"
#include "VulkanRenderer.h"
#include "VulkanUtility.h"
#define VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME "VK_LAYER_LUNARG_standard_validation"
#define VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME "VK_LAYER_LUNARG_assistant_layer"
#define VK_LAYER_LUNARG_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

namespace NekoEngine
{
    const bool EnableValidationLayers = true;

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

        if(EnableValidationLayers)
        {
         extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
         extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
         extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back("VK_KHR_portability_enumeration");
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

//        extensions.push_back("VK_KHR_swapchain");
//        extensions.push_back("VK_KHR_surface");
        // extensions.push_back("VK_KHR_DISPLAY");
        return extensions;
    }

    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

        if(func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pCallback);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }


    const std::vector<const char*> VulkanContext::GetRequiredLayers() const
    {
        std::vector<const char*> layers = {};

        if(EnableValidationLayers)
        {
            layers.emplace_back(VK_LAYER_LUNARG_VALIDATION_NAME);
        }

        return layers;
    }

    VulkanContext::~VulkanContext()
    {
//        VulkanRenderer::Flush
        vkDestroyDescriptorPool(GET_DEVICE(), VulkanRenderer::GetDescriptorPool(), nullptr);
        if(m_DebugCallback)
        {
            vkDestroyDebugReportCallbackEXT(vkInstance, m_DebugCallback, VK_NULL_HANDLE);
        }
        device.reset();
        vkDestroyInstance(vkInstance, nullptr);
    }


    void VulkanContext::Init()
    {
        LOG("Initializing Vulkan context");
        CreateInstance();

        device = MakeShared<VulkanDevice>();
        device->Init();

        SetupDebugCallback();

        LOG("Vulkan context initialized");
    }

    void VulkanContext::SetupDebugCallback()
    {
        if(!EnableValidationLayers) return;

        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        createInfo.pfnCallback                        = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(DebugCallback);

        VkResult result = CreateDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &m_DebugCallback);
        if(result != VK_SUCCESS)
        {
            LOG("[VULKAN] Failed to set up debug callback!");
        }
    }

    VkBool32 VulkanContext::DebugCallback(VkDebugReportFlagsEXT flags,
                                      VkDebugReportObjectTypeEXT objType,
                                      uint64_t sourceObj,
                                      size_t location,
                                      int32_t msgCode,
                                      const char* pLayerPrefix,
                                      const char* pMsg,
                                      void* userData)
    {
        // Select prefix depending on flags passed to the callback
        // Note that multiple flags may be set for a single validation message
        // Error that may result in undefined behaviour

        if(!flags)
            return VK_FALSE;

        if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            LOG_FORMAT("[VULKAN] - ERROR : [%s] Code %d  : %s", pLayerPrefix, msgCode, pMsg);
        }
        // Warnings may hint at unexpected / non-spec API usage
        if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            LOG_FORMAT("[VULKAN] - WARNING : [%s] Code %d  : %s", pLayerPrefix, msgCode, pMsg);
        }
        // May indicate sub-optimal usage of the API
        if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            LOG_FORMAT("[VULKAN] - PERFORMANCE : [%s] Code %d  : %s", pLayerPrefix, msgCode, pMsg);
        }
        // Informal messages that may become handy during debugging
        if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            LOG_FORMAT("[VULKAN] - INFO : [%s] Code %d  : %s", pLayerPrefix, msgCode, pMsg);
        }
        // Diagnostic info from the Vulkan loader and layers
        // Usually not helpful in terms of API usage, but may help to debug layer and loader problems
        if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            LOG_FORMAT("[VULKAN] - DEBUG : [%s] Code %d  : %s", pLayerPrefix, msgCode, pMsg);
        }

        return VK_FALSE;
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

        uint32_t sdkVersion    = VK_HEADER_VERSION_COMPLETE;
        uint32_t driverVersion = 0;
        auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

        if(enumerateInstanceVersion)
        {
            enumerateInstanceVersion(&driverVersion);
        }
        else
        {
            driverVersion = VK_API_VERSION_1_0;
        }

        appInfo.apiVersion = Maths::Min(sdkVersion, driverVersion);

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "NekoEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NekoEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);


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

//        VulkanUtility::Init();
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