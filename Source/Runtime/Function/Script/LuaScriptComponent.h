#pragma once

#include "Core.h"
#include "sol/forward.hpp"
#include "Level/Level.h"
#include "Entity/Entity.h"
#include "Engine.h"
#include "File/VirtualFileSystem.h"
namespace NekoEngine
{

    class LuaScriptComponent
    {
    private:
        Level* m_Level = nullptr;
        std::string m_FileName;
        std::map<int, std::string> m_Errors;

        SharedPtr<sol::environment> m_Env;
        SharedPtr<sol::protected_function> m_OnInitFunc;
        SharedPtr<sol::protected_function> m_UpdateFunc;
        SharedPtr<sol::protected_function> m_OnReleaseFunc;

        SharedPtr<sol::protected_function> m_Phys2DBeginFunc;
        SharedPtr<sol::protected_function> m_Phys3DBeginFunc;
        SharedPtr<sol::protected_function> m_Phys2DEndFunc;
        SharedPtr<sol::protected_function> m_Phys3DEndFunc;

    public:
        LuaScriptComponent();
        LuaScriptComponent(const std::string& fileName, Level* level);
        ~LuaScriptComponent();

        void Init();
        void OnInit();
        void OnUpdate(float dt);
        void Reload();
        void Load(const std::string& fileName);
        Entity GetCurrentEntity();

        // For accessing this component in lua
        void SetThisComponent();

        void LoadScript(const std::string& fileName);

        void OnCollision2DBegin();
        void OnCollision2DEnd();

        void OnCollision3DBegin();
        void OnCollision3DEnd();

        const sol::environment& GetSolEnvironment() const
        {
            return *m_Env;
        }
        const std::string& GetFilePath() const
        {
            return m_FileName;
        }

        void SetFilePath(const std::string& path)
        {
            m_FileName = path;
        }

        const std::map<int, std::string>& GetErrors() const
        {
            return m_Errors;
        }

        bool Loaded()
        {
            return m_Env.get() != nullptr;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            std::string newPath;
            VirtualFileSystem::AbsoulePathToVFS(m_FileName, newPath);
            archive(cereal::make_nvp("FilePath", newPath));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            m_Level = gEngine->GetLevelManager()->GetCurrentLevel();
            archive(cereal::make_nvp("FilePath", m_FileName));
            Init();
        }
    };

} // NekoEngine

