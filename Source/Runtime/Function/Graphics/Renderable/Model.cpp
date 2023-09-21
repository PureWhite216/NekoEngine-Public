//
// Created by 80529 on 2023/7/20.
//
#include "File/VirtualFileSystem.h"
#include "Model.h"
#include "StringUtility.h"

namespace NekoEngine
{
    Model::Model(const String& _filePath) : filePath(_filePath), primitiveType(PrimitiveType::File)
    {
        LoadModel(filePath);
    }

    Model::Model(const SharedPtr<Mesh> &mesh, PrimitiveType type) : filePath("Primitive"), primitiveType(type)
    {
        meshes.push_back(mesh);
    }

    Model::Model(PrimitiveType type) : filePath("Primitive"), primitiveType(type)
    {
        meshes.push_back(SharedPtr<Mesh>(MeshFactory::CreatePrimative(type)));
    }

    void Model::LoadModel(const String &path)
    {
        //TODO load model
        String resolvedPath;
        if(!VirtualFileSystem::ResolvePhysicalPath(path, resolvedPath))
        {
            LOG_FORMAT("Model::LoadModel: Can't resolve path: %s", path.c_str());
        }

        const String fileExtension = StringUtility::GetFilePathExtension(resolvedPath);

        if(fileExtension == "fbx" || fileExtension == "FBX")
        {
            LoadFBX(resolvedPath);
        }
        else if(fileExtension == "obj" || fileExtension == "OBJ")
        {
            LOG_FORMAT("Model::LoadModel: Unsupport file extension: %s", fileExtension.c_str());
        }
        else if(fileExtension == "gltf")
        {
            LOG_FORMAT("Model::LoadModel: Unsupport file extension: %s", fileExtension.c_str());
        }
        else
        {
            LOG_FORMAT("Model::LoadModel: Unsupport file extension: %s", fileExtension.c_str());
        }


        LOG_FORMAT("Model::LoadModel: Load model: %s", path.c_str());
    }
}
