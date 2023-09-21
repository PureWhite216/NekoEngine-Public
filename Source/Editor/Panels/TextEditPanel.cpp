#include "TextEditPanel.h"
#include "StringUtility.h"
#include "Script/LuaManager.h"
#include "File/FileSystem.h"
#include "Editor.h"

namespace NekoEngine
{
    TextEditPanel::TextEditPanel(const std::string& filePath)
            : m_FilePath(filePath)
    {
        m_Name           = "Text Editor###textEdit";
        m_SimpleName     = "TextEdit";
        m_OnSaveCallback = NULL;
        editor.SetCustomIdentifiers({});

        auto extension = StringUtility::GetFilePathExtension(m_FilePath);

        if(extension == "lua" || extension == "Lua")
        {
            auto lang = TextEditor::LanguageDefinition::Lua();
            editor.SetLanguageDefinition(lang);

            auto& customIdentifiers = LuaManager::GetIdentifiers();
            TextEditor::Identifiers identifiers;

            for(auto& k : customIdentifiers)
            {
                TextEditor::Identifier id;
                id.mDeclaration = "Engine function";
                identifiers.insert(std::make_pair(k, id));
            }

            editor.SetCustomIdentifiers(identifiers);
        }
        else if(extension == "cpp")
        {
            auto lang = TextEditor::LanguageDefinition::CPlusPlus();
            editor.SetLanguageDefinition(lang);
        }
        else if(extension == "glsl" || extension == "vert" || extension == "frag")
        {
            auto lang = TextEditor::LanguageDefinition::GLSL();
            editor.SetLanguageDefinition(lang);
        }

        auto string = FileSystem::ReadTextFile(m_FilePath);
        editor.SetText(string);
        editor.SetShowWhitespaces(false);
    }

    void TextEditPanel::SetErrors(const std::map<int, std::string>& errors)
    {
        editor.SetErrorMarkers(errors);
    }

    void TextEditPanel::OnImGui()
    {
        if(GET_INPUT()->GetKeyHeld(KeyCode::LeftSuper) || (GET_INPUT()->GetKeyHeld(KeyCode::LeftControl)))
        {
            if(GET_INPUT()->GetKeyPressed(KeyCode::S))
            {
                auto textToSave = editor.GetText();
                FileSystem::WriteTextFile(m_FilePath, textToSave);
            }
        }

        auto cpos = editor.GetCursorPosition();
        ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if(ImGui::Begin(m_Name.c_str(), &m_Active, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar))
        {
            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("File"))
                {
                    if(ImGui::MenuItem("Save", "CTRL+S"))
                    {
                        auto textToSave = editor.GetText();
                        FileSystem::WriteTextFile(m_FilePath, textToSave);
                        if(m_OnSaveCallback)
                            m_OnSaveCallback();
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu("Edit"))
                {
                    bool ro = editor.IsReadOnly();
                    if(ImGui::MenuItem("Read-only mode", nullptr, &ro))
                        editor.SetReadOnly(ro);
                    ImGui::Separator();

                    if(ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                        editor.Undo();
                    if(ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                        editor.Redo();

                    ImGui::Separator();

                    if(ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                        editor.Copy();
                    if(ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                        editor.Cut();
                    if(ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                        editor.Delete();
                    if(ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                        editor.Paste();

                    ImGui::Separator();

                    if(ImGui::MenuItem("Select all", nullptr, nullptr))
                        editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    if(ImGui::MenuItem("Close", nullptr, nullptr))
                    {
                        OnClose();
                        ImGui::EndMenu();
                        ImGui::EndMenuBar();
                        ImGui::End();
                        return;
                    }

                    ImGui::EndMenu();
                }

                if(ImGui::BeginMenu("View"))
                {
                    if(ImGui::MenuItem("Dark palette"))
                        editor.SetPalette(TextEditor::GetDarkPalette());
                    if(ImGui::MenuItem("Light palette"))
                        editor.SetPalette(TextEditor::GetLightPalette());
                    if(ImGui::MenuItem("Retro blue palette"))
                        editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins", editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), StringUtility::GetFileName(m_FilePath).c_str());

            if(ImGui::IsItemActive())
            {
                if(GET_INPUT()->GetKeyHeld(KeyCode::LeftControl) && GET_INPUT()->GetKeyPressed(KeyCode::S))
                {
                    auto textToSave = editor.GetText();
                    FileSystem::WriteTextFile(m_FilePath, textToSave);
                }
            }

            editor.Render(m_Name.c_str());
        }
        ImGui::End();
    }

    void TextEditPanel::OnClose()
    {
        m_Editor->RemovePanel(this);
    }
} // NekoEngine