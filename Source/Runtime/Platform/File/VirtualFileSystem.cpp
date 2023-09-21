#include "VirtualFileSystem.h"
#include "FileSystem.h"

namespace NekoEngine
{
    std::map<std::string, std::vector<std::string>, std::less<void>> VirtualFileSystem::mountPoints = {};

    void VirtualFileSystem::Mount(const std::string &virtualPath, const std::string &physicalPath, bool replace)
    {
        if(replace)
            mountPoints[virtualPath].clear();

        mountPoints[virtualPath].push_back(physicalPath);
    }

    void VirtualFileSystem::Unmount(const std::string &path)
    {
        mountPoints[path].clear();
    }

    bool VirtualFileSystem::ResolvePhysicalPath(const std::string &path, std::string &outPhysicalPath, bool folder)
    {
        const std::string &updatedPath = path;
        // std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        if(!(path[0] == '/' && path[1] == '/'))
        {
            outPhysicalPath = path;
            return folder ? FileSystem::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        static std::string delimiter = "/";
        auto slash = updatedPath.find_first_of(delimiter, 2);
        auto virtualDir = std::string_view(updatedPath);
        virtualDir = virtualDir.substr(2, slash - 2);

        auto it = mountPoints.find(virtualDir);
        if(it == mountPoints.end() || it->second.empty())
        {
            outPhysicalPath = updatedPath;
            return folder ? FileSystem::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        const std::string remainder = updatedPath.substr(virtualDir.size() + 2, updatedPath.size() - virtualDir.size());
        for(const std::string &physicalPath: it->second)
        {
            const std::string newPath = physicalPath + /* "/" +*/ remainder;
            if(folder ? FileSystem::FolderExists(newPath) : FileSystem::FileExists(newPath))
            {
                outPhysicalPath = newPath;
                return true;
            }
        }
        return false;
    }

    uint8_t* VirtualFileSystem::ReadFile(const std::string &path)
    {

        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
    }

    std::string VirtualFileSystem::ReadTextFile(const std::string &path)
    {

        std::string physicalPath;
        bool result = ResolvePhysicalPath(path, physicalPath);
        if(result)
        {
            return FileSystem::ReadTextFile(physicalPath);
        }
        else
        {
            return "";
        }
        // return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : "";
    }

    bool VirtualFileSystem::WriteFile(const std::string &path, uint8_t* buffer, uint32_t size)
    {

        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) && FileSystem::WriteFile(physicalPath, buffer, size);
    }

    bool VirtualFileSystem::WriteTextFile(const std::string &path, const std::string &text)
    {

        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) && FileSystem::WriteTextFile(physicalPath, text);
    }

    bool VirtualFileSystem::AbsoulePathToVFS(const std::string &path, std::string &outVirtualFileSystemPath,
                                             bool folder)
    {

        std::string updatedPath = path;
        std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        for(auto const &[key, val]: mountPoints)
        {
            for(auto &VirtualFileSystemPath: val)
            {
                if(updatedPath.find(VirtualFileSystemPath) != std::string::npos)
                {
                    std::string newPath = updatedPath;
                    std::string newPartPath = "//" + key;
                    newPath.replace(0, VirtualFileSystemPath.length(), newPartPath);
                    outVirtualFileSystemPath = newPath;
                    return true;
                }
            }
        }

        outVirtualFileSystemPath = updatedPath;
        return false;
    }
} // NekoEngine