#define GLFW_EXPOSE_NATIVE_WIN32
#include "WindowsOS.h"
#include <Windows.h>
//#include "windef.h"
#include "Window/Window.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <filesystem>
#include <shellapi.h>
#include <dwmapi.h>
#include <winuser.h>

namespace NekoEngine
{
    void WindowsOS::Run()
    {
        //TODO: ???
    }

    void WindowsOS::Init()
    {
        //
    }

//    SystemMemoryInfo MemoryManager::GetSystemInfo()
//    {
//        MEMORYSTATUSEX status;
//        status.dwLength = sizeof(MEMORYSTATUSEX);
//        GlobalMemoryStatusEx(&status);
//
//        SystemMemoryInfo result = {
//                (int64_t)status.ullAvailPhys,
//                (int64_t)status.ullTotalPhys,
//
//                (int64_t)status.ullAvailVirtual,
//                (int64_t)status.ullTotalVirtual
//        };
//        return result;
//    }

    std::string WindowsOS::GetExecutablePath()
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);

        std::string convertedString = std::filesystem::path(path).string();
        std::replace(convertedString.begin(), convertedString.end(), '\\', '/');

        return convertedString;
    }

    void WindowsOS::OpenFileLocation(const std::filesystem::path& path)
    {
        ShellExecuteA(NULL, "open", std::filesystem::is_directory(path) ? path.string().c_str() : path.parent_path().string().c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void WindowsOS::OpenFileExternal(const std::filesystem::path& path)
    {
        ShellExecuteA(NULL, "open", path.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void WindowsOS::OpenURL(const std::string& url)
    {
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

} // NekoEngine