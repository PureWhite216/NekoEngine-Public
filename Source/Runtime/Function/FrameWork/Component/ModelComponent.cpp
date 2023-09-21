#include "Engine.h"
#include "ModelComponent.h"

namespace NekoEngine
{
    ModelComponent::ModelComponent(const std::string &path)
    {
        LoadFromLibrary(path);
    }

    void ModelComponent::LoadFromLibrary(const String path)
    {
        model = gEngine->GetModelLibrary()->GetResource(path);
    }
} // NekoEngine