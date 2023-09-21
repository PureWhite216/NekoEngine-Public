#pragma once

#include <cereal/cereal.hpp>
#include "Core.h"
#include "Asset.h"
#include "Mesh.h"
#include "GlmSerizlization.h"
#include "File/VirtualFileSystem.h"
#include "MeshFactory.h"


namespace NekoEngine
{
    class Model : public Asset
    {
    private:
        PrimitiveType primitiveType = PrimitiveType::None;
        ArrayList<SharedPtr<Mesh>> meshes;
        String filePath;
    public:
        Model() = default;
        Model(const String& _filePath);
        Model(const SharedPtr<Mesh>& mesh, PrimitiveType type);
        Model(PrimitiveType type);

        ~Model() = default;

        void LoadModel(const String& path);

        ArrayList<SharedPtr<Mesh>>& GetMeshes() { return meshes; }
        void AddMesh(SharedPtr<Mesh> mesh) { meshes.push_back(mesh); }

        const std::string& GetFilePath() const { return filePath; }
        PrimitiveType GetPrimitiveType() { return primitiveType; }
        void SetPrimitiveType(PrimitiveType type) { primitiveType = type; }
        SET_ASSET_TYPE(AssetType::Model);

        template <typename Archive>
        void save(Archive& archive) const
        {
            if(meshes.size() > 0)
            {
                std::string newPath;
                VirtualFileSystem::AbsoulePathToVFS(filePath, newPath);

                auto material = std::unique_ptr<Material>(meshes.front()->GetMaterial().get());
                archive(cereal::make_nvp("PrimitiveType", primitiveType), cereal::make_nvp("FilePath", newPath), cereal::make_nvp("Material", material));
                material.release();
            }
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            auto material = std::unique_ptr<Material>();

            archive(cereal::make_nvp("PrimitiveType", primitiveType), cereal::make_nvp("FilePath", filePath), cereal::make_nvp("Material", material));

            meshes.clear();

            if(primitiveType != PrimitiveType::File)
            {
                meshes.push_back(SharedPtr<Mesh>(MeshFactory::CreatePrimative(primitiveType)));
                meshes.back()->SetMaterial(SharedPtr<Material>(material.get()));
                material.release();
            }
            else
            {
                LoadModel(filePath);
                // TODO: This should load material changes from editor
                // meshes.back()->SetMaterial(SharedPtr<Material>(material.get()));
                // material.release();
            }
        }

    private:
        void LoadOBJ(const String& path);
        void LoadGLTF(const String & path);
        void LoadFBX(const String& path);
    };
}