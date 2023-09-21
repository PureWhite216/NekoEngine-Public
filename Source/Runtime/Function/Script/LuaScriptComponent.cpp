#include "LuaScriptComponent.h"
#include "StringUtility.h"
#include "Entity/EntityManager.h"

namespace NekoEngine
{
    LuaScriptComponent::LuaScriptComponent()
    {
        m_Level    = nullptr;
        m_FileName = "";
        m_Env      = nullptr;
        // m_UUID = UUID();
    }
    LuaScriptComponent::LuaScriptComponent(const std::string& fileName, Level* level)
    {
        m_Level    = level;
        m_FileName = fileName;
        m_Env      = nullptr;
        // m_UUID = UUID();

        Init();
    }

    void LuaScriptComponent::Init()
    {
        LoadScript(m_FileName);
    }

    LuaScriptComponent::~LuaScriptComponent()
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if(releaseFunc.valid())
                releaseFunc.call();
        }
    }

    void LuaScriptComponent::LoadScript(const std::string& fileName)
    {
        m_FileName = fileName;
        std::string physicalPath;
        if(!VirtualFileSystem::ResolvePhysicalPath(fileName, physicalPath))
        {
            LOG_FORMAT("Failed to Load Lua script %s", fileName.c_str());
            m_Env = nullptr;
            return;
        }

        VirtualFileSystem::AbsoulePathToVFS(m_FileName, m_FileName);

        m_Env = MakeShared<sol::environment>(gEngine->GetLuaManager()->GetState(), sol::create, gEngine->GetLuaManager()->GetState().globals());

        auto loadFileResult = gEngine->GetLuaManager()->GetState().script_file(physicalPath, *m_Env, sol::script_pass_on_error);
        if(!loadFileResult.valid())
        {
            sol::error err = loadFileResult;
            LOG_FORMAT("Failed to Execute Lua script %s", physicalPath.c_str());
            std::string filename = StringUtility::GetFileName(m_FileName);
            std::string error    = std::string(err.what());

            int line              = 1;
            auto linepos          = error.find(".lua:");
            std::string errorLine = error.substr(linepos + 5); //+4 .lua: + 1
            auto lineposEnd       = errorLine.find(":");
            errorLine             = errorLine.substr(0, lineposEnd);
            line                  = std::stoi(errorLine);
            error                 = error.substr(linepos + errorLine.size() + lineposEnd + 4); //+4 .lua:

            m_Errors[line] = std::string(error);
        }
        else
            m_Errors = {};

        if(!m_Level)
            m_Level = gEngine->GetLevelManager()->GetCurrentLevel();

        (*m_Env)["CurrentLevel"] = m_Level;
        (*m_Env)["LuaComponent"] = this;

        m_OnInitFunc = MakeShared<sol::protected_function>((*m_Env)["OnInit"]);
        if(!m_OnInitFunc->valid())
            m_OnInitFunc.reset();

        m_UpdateFunc = MakeShared<sol::protected_function>((*m_Env)["OnUpdate"]);
        if(!m_UpdateFunc->valid())
            m_UpdateFunc.reset();

        m_Phys2DBeginFunc = MakeShared<sol::protected_function>((*m_Env)["OnCollision2DBegin"]);
        if(!m_Phys2DBeginFunc->valid())
            m_Phys2DBeginFunc.reset();

        m_Phys2DEndFunc = MakeShared<sol::protected_function>((*m_Env)["OnCollision2DEnd"]);
        if(!m_Phys2DEndFunc->valid())
            m_Phys2DEndFunc.reset();

        m_Phys3DBeginFunc = MakeShared<sol::protected_function>((*m_Env)["OnCollision3DBegin"]);
        if(!m_Phys3DBeginFunc->valid())
            m_Phys3DBeginFunc.reset();

        m_Phys3DEndFunc = MakeShared<sol::protected_function>((*m_Env)["OnCollision3DEnd"]);
        if(!m_Phys3DEndFunc->valid())
            m_Phys3DEndFunc.reset();

        gEngine->GetLuaManager()->GetState().collect_garbage();
    }

    void LuaScriptComponent::OnInit()
    {
        if(m_OnInitFunc)
        {
            sol::protected_function_result result = m_OnInitFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua Init function");
            }
        }
    }

    void LuaScriptComponent::OnUpdate(float dt)
    {
        if(m_UpdateFunc)
        {
            sol::protected_function_result result = m_UpdateFunc->call(dt);
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua OnUpdate");
            }
        }
    }

    void LuaScriptComponent::Reload()
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if(releaseFunc.valid())
                releaseFunc.call();
        }

        Init();
    }

    Entity LuaScriptComponent::GetCurrentEntity()
    {
        if(!m_Level)
            m_Level = gEngine->GetLevelManager()->GetCurrentLevel();

        auto entities = m_Level->GetEntityManager()->GetEntitiesWithType<LuaScriptComponent>();

        for(auto entity : entities)
        {
            LuaScriptComponent* comp = &entity.GetComponent<LuaScriptComponent>();
            if(comp->GetFilePath() == GetFilePath())
                return entity;
        }

        return Entity();
    }

    void LuaScriptComponent::SetThisComponent()
    {
        if(m_Env)
        {
            (*m_Env)["LuaComponent"] = this;
        }
    }

    void LuaScriptComponent::Load(const std::string& fileName)
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if(releaseFunc.valid())
                releaseFunc.call();
        }

        m_FileName = fileName;
        Init();
    }

    void LuaScriptComponent::OnCollision2DBegin()
    {
        if(m_Phys2DBeginFunc)
        {
            sol::protected_function_result result = m_Phys2DBeginFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua OnCollision2DBegin");
            }
        }
    }

    void LuaScriptComponent::OnCollision2DEnd()
    {
        if(m_Phys2DEndFunc)
        {
            sol::protected_function_result result = m_Phys2DEndFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua OnCollision2DEnd");
            }
        }
    }

    void LuaScriptComponent::OnCollision3DBegin()
    {
        if(m_Phys3DBeginFunc)
        {
            sol::protected_function_result result = m_Phys3DBeginFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua OnCollision3DBegin");
            }
        }
    }

    void LuaScriptComponent::OnCollision3DEnd()
    {
        if(m_Phys3DEndFunc)
        {
            sol::protected_function_result result = m_Phys3DEndFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                LOG("Failed to Execute Script Lua OnCollision3DEnd");
            }
        }
    }
} // NekoEngine