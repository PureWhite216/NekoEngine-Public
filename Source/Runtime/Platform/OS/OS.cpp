#include "OS.h"
#include "WindowsOS.h"

namespace NekoEngine
{
    OS* OS::s_Instance = nullptr;

    void OS::Create()
    {
        s_Instance = new WindowsOS();
    }

    void OS::Release()
    {
        delete s_Instance;
        s_Instance = nullptr;
    }
} // NekoEngine