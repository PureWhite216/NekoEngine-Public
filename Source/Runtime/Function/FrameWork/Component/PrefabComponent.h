#pragma once
#include "Core.h"

namespace NekoEngine
{

    class PrefabComponent
    {
    public:
        PrefabComponent(const std::string& path)
        {
            Path = path;
        }

        std::string Path;
        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(Path);
        }
    };

} // NekoEngine

