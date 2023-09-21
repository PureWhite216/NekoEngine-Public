
#pragma once
#include "Renderable/Model.h"
#include "GlmSerizlization.h"

namespace NekoEngine
{

    class ModelComponent
    {
    public:
        SharedPtr<Model> model;

    public:
        ModelComponent() = default;
        ModelComponent(const std::string& path);
        ModelComponent(const SharedPtr<Model>& modelRef) : model(modelRef) {}
        ModelComponent(PrimitiveType primitive) : model(MakeShared<Model>(primitive)){}

        void LoadFromLibrary(const String path);
        void LoadPrimitive(PrimitiveType primitive){ model = MakeShared<Model>(primitive); }

        template <typename Archive>
        void save(Archive& archive) const
        {
            if(!model || model->GetMeshes().size() == 0)
                return;
            {
                std::string newPath;

                if(model->GetPrimitiveType() == PrimitiveType::File)
                    VirtualFileSystem::AbsoulePathToVFS(model->GetFilePath(), newPath);
                else
                    newPath = "Primitive";

                // For now this saved material will be overriden by materials in the model file
                auto material = std::unique_ptr<Material>(model->GetMeshes().front()->GetMaterial().get());
                archive(cereal::make_nvp("PrimitiveType", model->GetPrimitiveType()), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                material.release();
            }
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            auto material = std::unique_ptr<Material>();

            std::string filePath;
            PrimitiveType primitiveType;

            archive(cereal::make_nvp("PrimitiveType", primitiveType), cereal::make_nvp("FilePath", filePath), cereal::make_nvp("Material", material));

            if(primitiveType != PrimitiveType::File)
            {
                model = MakeShared<Model>(primitiveType);
                model->GetMeshes().back()->SetMaterial(SharedPtr<Material>(material.get()));
                material.release();
            }
            else
            {
                LoadFromLibrary(filePath);
            }
        }
        
    };

} // NekoEngine

