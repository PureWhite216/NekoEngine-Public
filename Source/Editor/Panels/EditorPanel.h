#pragma once
#include "Core.h"
#include "Level/Level.h"

namespace NekoEngine
{
    class Editor;

    class EditorPanel
    {
    protected:
        bool m_Active = true;
        std::string m_Name;
        std::string m_SimpleName;
        Editor* m_Editor = nullptr;

    public:
        virtual ~EditorPanel() = default;

        const std::string& GetName() const
        {
            return m_Name;
        }
        const std::string& GetSimpleName() const
        {
            return m_SimpleName;
        }

        void SetEditor(Editor* editor)
        {
            m_Editor = editor;
        }
        Editor* GetEditor()
        {
            return m_Editor;
        }
        bool& Active()
        {
            return m_Active;
        }
        void SetActive(bool active)
        {
            m_Active = active;
        }
        virtual void OnImGui() = 0;
        virtual void OnNewLevel(Level* level)
        {
        }

        virtual void OnNewProject()
        {
        }

        virtual void OnRender()
        {
        }

        virtual void DestroyGraphicsResources()
        {
        }
    };
}