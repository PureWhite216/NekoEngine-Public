#pragma once
#include "EditorPanel.h"
#include "File/FileSystem.h"
#include "ImGui/Plugins/ImFileBrowser.h"

namespace NekoEngine
{

    class FileBrowserPanel : public EditorPanel
    {
    private:
        std::function<void(const std::string&)> m_Callback;
        ImGui::FileBrowser* m_FileBrowser;

    public:
        FileBrowserPanel();
        ~FileBrowserPanel();

        void Open();
        void OnImGui() override;
        void SetCurrentPath(const std::string& path);
        void SetOpenDirectory(bool value);
        void SetCallback(const std::function<void(const std::string&)>& callback)
        {
            m_Callback = callback;
        }

        bool IsOpen();
        void SetFileTypeFilters(const std::vector<const char*>& fileFilters);
        void ClearFileTypeFilters();
        std::filesystem::path& GetPath();

    };

} // NekoEngine

