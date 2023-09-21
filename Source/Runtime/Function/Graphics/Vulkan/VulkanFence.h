#include "Vk.h"

namespace NekoEngine
{

    class VulkanFence
    {
    private:
        VkFence handle;
        bool isSignaled;
    public:
        VulkanFence(bool signaled = false);
        ~VulkanFence();

        void Reset();
        bool Wait();
        void WaitAndReset();
        bool CheckState();
        VkFence GetHandle() const { return handle; }\

        bool IsSignaled();
        void SetSignaled(bool signaled) { isSignaled = signaled;}
    };

} // NekoEngine


