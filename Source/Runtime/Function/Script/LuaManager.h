#pragma once
#include "Core.h"
#include "sol/sol.hpp"

namespace NekoEngine
{
    class Level;

    class NEKO_EXPORT LuaManager
    {
    public:
        static std::vector<std::string> s_Identifiers;
        
    private:
        sol::state* m_State;

    public:
        LuaManager();
        ~LuaManager();

        void OnInit();
        void OnInit(Level* level);
        void OnUpdate(Level* level);

        void OnNewProject(const std::string& projectPath);

        void BindECSLua(sol::state& state);
        void BindLogLua(sol::state& state);
        void BindInputLua(sol::state& state);
        void BindLevelLua(sol::state& state);
        void BindAppLua(sol::state& state);

        static std::vector<std::string>& GetIdentifiers();

        sol::state& GetState()
        {
            return *m_State;
        }
    };

} // NekoEngine


