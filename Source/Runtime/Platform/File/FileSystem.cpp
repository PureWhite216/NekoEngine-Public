
// WINDOWS IMPLEMENTATION OF FILESYSTEM
// TODO: Cross Platform Implementation
#include "FileSystem.h"
#include "Windows.h"
#include <codecvt>
#include <locale>
#include <fstream>

namespace NekoEngine
{
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_t, wchar_t> strconverter;

    std::string WStringToString(std::wstring wstr)
    {
        return strconverter.to_bytes(wstr);
    }

    std::wstring StringToWString(std::string str)
    {
        return strconverter.from_bytes(str);
    }

    static HANDLE OpenFileForReading(const std::string &path)
    {
        auto t = StringToWString(path).c_str();
        auto t1 = reinterpret_cast<LPCSTR>(t);
        return CreateFile(reinterpret_cast<LPCSTR>(t), GENERIC_READ, FILE_SHARE_READ,
                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    }

    static int64_t GetFileSizeInternal(const HANDLE file)
    {
        LARGE_INTEGER size;
        GetFileSizeEx(file, &size);
        return size.QuadPart;
    }

    static bool ReadFileInternal(const HANDLE file, void* buffer, const int64_t size)
    {
        OVERLAPPED ol = {0};
        return ReadFileEx(file, buffer, static_cast<DWORD>(size), &ol, nullptr) != 0;
    }

    bool FileSystem::FileExists(const std::string &path)
    {
        DWORD dwAttrib = GetFileAttributes(reinterpret_cast<LPCSTR>(StringToWString(path).c_str()));
        return (dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    bool FileSystem::FolderExists(const std::string &path)
    {
        DWORD dwAttrib = GetFileAttributes(reinterpret_cast<LPCSTR>(StringToWString(path).c_str()));
        return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    int64_t FileSystem::GetFileSize(const std::string &path)
    {
        const HANDLE file = OpenFileForReading(path);
        if(file == INVALID_HANDLE_VALUE)
            return -1;
        int64_t result = GetFileSizeInternal(file);
        CloseHandle(file);

        return result;
    }

    bool FileSystem::ReadFile(const std::string &path, void* buffer, int64_t size)
    {
        const HANDLE file = OpenFileForReading(path);
        if(file == INVALID_HANDLE_VALUE)
            return false;

        if(size < 0)
            size = GetFileSizeInternal(file);

        bool result = ReadFileInternal(file, buffer, size);
        CloseHandle(file);
        return result;
    }

    uint8_t* FileSystem::ReadFile(const std::string &path)
    {
        if(!FileExists(path))
            return nullptr;

        const HANDLE file = OpenFileForReading(path);
        const int64_t size = GetFileSizeInternal(file);
        uint8_t* buffer = new uint8_t[static_cast<uint32_t>(size)];
        const bool result = ReadFileInternal(file, buffer, size);
        CloseHandle(file);
        if(!result)
            delete[] buffer;
        return result ? buffer : nullptr;
    }

    std::string FileSystem::ReadTextFile(const std::string &path)
    {
        if(!FileExists(path))
            return std::string();

        std::ifstream stream(path);

        std::string fileContent;
        std::string line;
        while(std::getline(stream, line))
        {                               // Read file line by line
            fileContent += line + "\n"; // Append each line to fileContent
        }

        stream.close();

        return fileContent;
    }

    bool FileSystem::WriteFile(const std::string &path, uint8_t* buffer, uint32_t size)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);

        if(!stream)
        {
            stream.close();
            return false;
        }

        stream.write((char*)buffer, size);
        stream.close();

        return true;
    }

    bool FileSystem::WriteTextFile(const std::string &path, const std::string &text)
    {
        return WriteFile(path, (uint8_t*)&text[0], (uint32_t)text.size());
    }
} // NekoEngine