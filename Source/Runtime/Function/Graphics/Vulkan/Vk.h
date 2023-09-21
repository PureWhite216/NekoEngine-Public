#pragma once

#define USE_VMA_ALLOCATOR
#define VK_NO_PROTOTYPES
//#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"
#include "volk.h"
#include "Core.h"

#define VK_CHECK_RESULT(f, message) \
{ \
    VkResult res = (f); \
    if (res != VK_SUCCESS) \
    { \
        RUNTIME_ERROR("Vulkan Error: (message)"); \
    } \
}

inline PFN_vkCmdBeginDebugUtilsLabelEXT fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT;