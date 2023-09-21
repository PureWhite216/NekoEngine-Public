#pragma once
#include "Core.h"
#include "Level.h"
namespace NekoEngine
{

    class LevelManager
    {
    private:
        bool m_SwitchingLevels = false;
        int m_QueuedLevelIndex = -1;

    protected:
        uint32_t m_LevelIdx;
        Level* m_CurrentLevel;
        std::vector<SharedPtr<Level>> m_vpAllLevels;
        std::vector<std::string> m_LevelFilePaths;
        std::vector<std::string> m_LevelFilePathsToLoad;

    public:
        LevelManager();
        virtual ~LevelManager();

        // Jump to the next scene in the list or first scene if at the end
        void SwitchLevel();

        // Jump to scene index (stored in order they were originally added starting at zero)
        void SwitchLevel(int idx);

        // Jump to scene name
        void SwitchLevel(const std::string& name);

        void ApplyLevelSwitch();

        // Get currently active scene (returns NULL if no scenes yet added)
        inline Level* GetCurrentLevel() const
        {
            return m_CurrentLevel;
        }

        // Get currently active scene's index (return 0 if no scenes yet added)
        inline uint32_t GetCurrentLevelIndex() const
        {
            return m_LevelIdx;
        }

        // Get total number of enqueued scenes
        inline uint32_t LevelCount() const
        {
            return static_cast<uint32_t>(m_vpAllLevels.size());
        }

        std::vector<std::string> GetLevelNames();
        const std::vector<SharedPtr<Level>>& GetLevels() const
        {
            return m_vpAllLevels;
        }

        void SetSwitchLevel(bool switching)
        {
            m_SwitchingLevels = switching;
        }
        bool GetSwitchingLevel() const
        {
            return m_SwitchingLevels;
        }

        int EnqueueLevelFromFile(const std::string& filePath);
        void EnqueueLevel(Level* level);

        template <class T>
        void EnqueueLevel(const std::string& name)
        {
            m_vpAllLevels.emplace_back(MakeShared<T>(name));
        }

        const std::vector<std::string>& GetLevelFilePaths();

        void AddFileToLoadList(const std::string& filePath)
        {
            m_LevelFilePathsToLoad.push_back(filePath);
        }

        void LoadCurrentList();
    };

} // NekoEngine

