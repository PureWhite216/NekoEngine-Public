#pragma once
#include "OS.h"

namespace NekoEngine
{

    class WindowsOS : public OS
    {
    public:
        WindowsOS()  = default;
        ~WindowsOS() = default;

        void Init();
        void Run() override;
        std::string GetExecutablePath() override;

        void OpenFileLocation(const std::filesystem::path& path) override;
        void OpenFileExternal(const std::filesystem::path& path) override;
        void OpenURL(const std::string& url) override;
    };

} // NekoEngine
