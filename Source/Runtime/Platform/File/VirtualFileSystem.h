#pragma once
#include "Core.h"
namespace NekoEngine
{

    class VirtualFileSystem
    {
    private:
        static std::map<std::string, std::vector<std::string>, std::less<void>> mountPoints;
    public:
        static void Mount(const String& virtualPath, const String& physicalPath, bool replace = false);
        static void Unmount(const String& path);
        static bool ResolvePhysicalPath(const String& path, String& outPhysicalPath, bool folder = false);
        static bool AbsoulePathToVFS(const String& path, String& outVFSPath, bool folder = false);
        static inline String AbsoulePathToVFS(const String& path, bool folder = false)
        {
            String returnString;
            AbsoulePathToVFS(path, returnString, folder);
            return returnString;
        }

        static uint8_t* ReadFile(const String& path);
        static String ReadTextFile(const String& path);

        static bool WriteFile(const String& path, uint8_t* buffer, uint32_t size);
        static bool WriteTextFile(const String& path, const String& text);
    };

} // NekoEngine
