#pragma once
#include "Core.h"
#include "filesystem"

namespace NekoEngine
{

    class OS
    {
    protected:
        static OS* s_Instance;

    public:
        OS()          = default;
        virtual ~OS() = default;

        virtual void Run() = 0;

        static void Create();
        static void Release();
        static void SetInstance(OS* instance)
        {
            s_Instance = instance;
        }

        static OS* Instance()
        {
            return s_Instance;
        }

        virtual std::string GetCurrentWorkingDirectory() { return std::string(""); };
        virtual std::string GetExecutablePath() = 0;
        virtual std::string GetAssetPath()
        {
            return "";
        };
        virtual void Vibrate() const {};
//        virtual void SetTitleBarColour(const glm::vec4& colour, bool dark = true) {};

        // Mobile only
        virtual void ShowKeyboard() {};
        virtual void HideKeyboard() {};
        virtual void Delay(uint32_t usec) {};

        // Needed for MaxOS
        virtual void MaximiseWindow() { }

        virtual void OpenFileLocation(const std::filesystem::path& path) { }
        virtual void OpenFileExternal(const std::filesystem::path& path) { }
        virtual void OpenURL(const std::string& url) { }
    };

} // NekoEngine
