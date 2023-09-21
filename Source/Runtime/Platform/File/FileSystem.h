#pragma once
#include "Core.h"
namespace NekoEngine
{
    enum class FileOpenFlags : uint8_t
    {
        READ,
        WRITE,
        READ_WRITE,
        WRITE_READ
    };

    class FileSystem
    {
    private:
        static std::map<String, ArrayList<String>, std::less<void>> mountPoints;
    public:
        static bool FileExists(const String& path);
        static bool FolderExists(const String& path);
        static int64_t GetFileSize(const String& path);

        static uint8_t* ReadFile(const String& path);
        static bool ReadFile(const String& path, void* buffer, int64_t size = -1);
        static String ReadTextFile(const String& path);

        static bool WriteFile(const String& path, uint8_t* buffer, uint32_t size);
        static bool WriteTextFile(const String& path, const String& text);

        static String GetWorkingDirectory();

        static bool IsRelativePath(const char* path)
        {
            if(!path || path[0] == '/' || path[0] == '\\')
            {
                return false;
            }

            if(strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':')
            {
                return false;
            }

            return true;
        }
        static bool IsAbsolutePath(const char* path)
        {
            if(!path)
            {
                return false;
            }

            return !IsRelativePath(path);
        }
        static const char* GetFileOpenModeString(FileOpenFlags flag)
        {
            if(flag == FileOpenFlags::READ)
            {
                return "rb";
            }
            else if(flag == FileOpenFlags::WRITE)
            {
                return "wb";
            }
            else if(flag == FileOpenFlags::READ_WRITE)
            {
                return "rb+";
            }
            else if(flag == FileOpenFlags::WRITE_READ)
            {
                return "wb+";
            }
            else
            {
                LOG("Invalid open flag");
                return "rb";
            }
        }

        static void Mount(const String& virtualPath, const String& physicalPath, bool replace = false);
        static void Unmount(const String& path);
        static bool ResolvePhysicalPath(const String& path, String& outPhysicalPath, bool folder = false);
        static bool AbsoulePathToVFS(const String& path, String& outVFSPath, bool folder = false);
        inline static String AbsoulePathToVFS(const String& path, bool folder = false)
        {
            String returnString;
            AbsoulePathToVFS(path, returnString, folder);
            return returnString;
        }
    };

} // NekoEngine

