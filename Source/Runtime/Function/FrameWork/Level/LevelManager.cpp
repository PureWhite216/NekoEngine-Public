#include "LevelManager.h"
#include "Engine.h"
#include "PhysicsEngine.h"
#include "StringUtility.h"
#include "File/VirtualFileSystem.h"

namespace NekoEngine
{
    LevelManager::LevelManager()
            : m_LevelIdx(0)
            , m_CurrentLevel(nullptr)
    {
    }

    LevelManager::~LevelManager()
    {
        m_LevelIdx = 0;

        if(m_CurrentLevel)
        {
            LOG_FORMAT("[LevelManager] - Exiting level : %s", m_CurrentLevel->GetName().c_str());
            m_CurrentLevel->OnCleanup();
        }

        m_vpAllLevels.clear();
    }

    void LevelManager::SwitchLevel()
    {
        SwitchLevel((m_LevelIdx + 1) % m_vpAllLevels.size());
    }

    void LevelManager::SwitchLevel(int idx)
    {
        m_QueuedLevelIndex = idx;
        m_SwitchingLevels  = true;
    }

    void LevelManager::SwitchLevel(const std::string& name)
    {
        bool found        = false;
        m_SwitchingLevels = true;
        uint32_t idx      = 0;
        for(uint32_t i = 0; !found && i < m_vpAllLevels.size(); ++i)
        {
            if(m_vpAllLevels[i]->GetName() == name)
            {
                found = true;
                idx   = i;
                break;
            }
        }

        if(found)
        {
            SwitchLevel(idx);
        }
        else
        {
            LOG_FORMAT("[LevelManager] - Unknown Level Alias : %s", name.c_str());
        }
    }

    void LevelManager::ApplyLevelSwitch()
    {
        if(m_SwitchingLevels == false)
        {
            if(m_CurrentLevel)
                return;

            if(m_vpAllLevels.empty())
                m_vpAllLevels.push_back(MakeShared<Level>("NewLevel"));

            m_QueuedLevelIndex = 0;
        }

        if(m_QueuedLevelIndex < 0 || m_QueuedLevelIndex >= static_cast<int>(m_vpAllLevels.size()))
        {
            LOG_FORMAT("[LevelManager] - Invalid Level Index : %d", m_QueuedLevelIndex);
            m_QueuedLevelIndex = 0;
        }


        // Clear up old level
        if(m_CurrentLevel)
        {
            LOG_FORMAT("[LevelManager] - Exiting level : %s", m_CurrentLevel->GetName().c_str());
            gEngine->GetSystem<PhysicsEngine>()->SetPaused(true);

            m_CurrentLevel->OnCleanup();
            gEngine->OnExitLevel();
        }

        m_LevelIdx     = m_QueuedLevelIndex;
        m_CurrentLevel = m_vpAllLevels[m_QueuedLevelIndex].get();

        // Initialise new level
        gEngine->GetSystem<PhysicsEngine>()->SetDefaults();
        gEngine->GetSystem<PhysicsEngine>()->SetPaused(false);

        std::string physicalPath;
        if(VirtualFileSystem::ResolvePhysicalPath("//Levels/" + m_CurrentLevel->GetName() + ".lsn", physicalPath))
        {
            auto newPath = StringUtility::RemoveName(physicalPath);
            m_CurrentLevel->Deserialise(newPath, false);
        }

        auto screenSize = gEngine->GetWindowSize();
        m_CurrentLevel->SetScreenSize(static_cast<uint32_t>(screenSize.x), static_cast<uint32_t>(screenSize.y));

        if(gEngine->GetEditorState() == EditorState::Play)
            m_CurrentLevel->Init();

        gEngine->OnNewLevel(m_CurrentLevel);

        LOG_FORMAT("[LevelManager] - Level switched to : %s", m_CurrentLevel->GetName().c_str());

        m_SwitchingLevels = false;
    }

    std::vector<std::string> LevelManager::GetLevelNames()
    {
        std::vector<std::string> names;

        for(auto& level : m_vpAllLevels)
        {
            names.push_back(level->GetName());
        }

        return names;
    }

    int LevelManager::EnqueueLevelFromFile(const std::string& filePath)
    {
        auto found = std::find(m_LevelFilePaths.begin(), m_LevelFilePaths.end(), filePath);
        if(found != m_LevelFilePaths.end())
            return int(found - m_LevelFilePaths.begin());

        m_LevelFilePaths.push_back(filePath);

        auto name  = StringUtility::RemoveFilePathExtension(StringUtility::GetFileName(filePath));
        auto level = new Level(name);
        EnqueueLevel(level);
        return int(m_vpAllLevels.size()) - 1;
    }

    void LevelManager::EnqueueLevel(Level* level)
    {
        m_vpAllLevels.push_back(SharedPtr<Level>(level));
        LOG_FORMAT("[LevelManager] - Enqueued level : %s", level->GetName().c_str());
    }

    void LevelManager::LoadCurrentList()
    {
        for(auto& filePath : m_LevelFilePathsToLoad)
        {
            std::string newPath;
            VirtualFileSystem::AbsoulePathToVFS(filePath, newPath);
            EnqueueLevelFromFile(filePath);
        }

        m_LevelFilePathsToLoad.clear();
    }

    const std::vector<std::string>& LevelManager::GetLevelFilePaths()
    {
        m_LevelFilePaths.clear();
        for(auto level : m_vpAllLevels)
            m_LevelFilePaths.push_back("//Levels/" + level->GetName());
        return m_LevelFilePaths;
    }





} // NekoEngine