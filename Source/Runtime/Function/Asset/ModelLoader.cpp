#include "ModelLoader.h"
#include "openfbx/ofbx.h"
#include "RHI/Texture.h"
#include "StringUtility.h"
#include "File/FileSystem.h"
#include "Renderable/Material.h"
#include "RHI/RHIFactory.h"
#include "AssetManager.h"
#include "Renderable/Model.h"
#include "Math/Transform.h"
#include "BoundingBox.h"
#include "Engine.h"

namespace NekoEngine
{
    const uint32_t MAX_PATH_LENGTH = 260;

    String modelDirectory;

    enum class Orientation
    {
        Y_UP,
        Z_UP,
        Z_MINUS_UP,
        X_MINUS_UP,
        X_UP
    };

    Orientation orientation = Orientation::Y_UP;
    float fbx_scale         = 1.f;

    static ofbx::Vec3 operator-(const ofbx::Vec3& a, const ofbx::Vec3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    static ofbx::Vec2 operator-(const ofbx::Vec2& a, const ofbx::Vec2& b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    glm::vec3 FixOrientation(const glm::vec3& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return {v.x, v.y, v.z};
        case Orientation::Z_UP:
            return {v.x, v.z, -v.y};
        case Orientation::Z_MINUS_UP:
            return {v.x, -v.z, v.y};
        case Orientation::X_MINUS_UP:
            return {v.y, -v.x, v.z};
        case Orientation::X_UP:
            return {-v.y, v.x, v.z};
        }
        return {v.x, v.y, v.z};
    }


    glm::quat FixOrientation(const glm::quat& v)
    {
        switch(orientation)
        {
        case Orientation::Y_UP:
            return glm::quat(v.x, v.y, v.z, v.w);
        case Orientation::Z_UP:
            return glm::quat(v.x, v.z, -v.y, v.w);
        case Orientation::Z_MINUS_UP:
            return glm::quat(v.x, -v.z, v.y, v.w);
        case Orientation::X_MINUS_UP:
            return glm::quat(v.y, -v.x, v.z, v.w);
        case Orientation::X_UP:
            return glm::quat(-v.y, v.x, v.z, v.w);
        }
        return glm::quat(v.x, v.y, v.z, v.w);
    }

    static void computeTangents(ofbx::Vec3* out, int vertex_count, const ofbx::Vec3* vertices, const ofbx::Vec3* normals, const ofbx::Vec2* uvs)
    {
        for(int i = 0; i < vertex_count; i += 3)
        {
            const ofbx::Vec3 v0  = vertices[i + 0];
            const ofbx::Vec3 v1  = vertices[i + 1];
            const ofbx::Vec3 v2  = vertices[i + 2];
            const ofbx::Vec2 uv0 = uvs[i + 0];
            const ofbx::Vec2 uv1 = uvs[i + 1];
            const ofbx::Vec2 uv2 = uvs[i + 2];

            const ofbx::Vec3 dv10  = v1 - v0;
            const ofbx::Vec3 dv20  = v2 - v0;
            const ofbx::Vec2 duv10 = uv1 - uv0;
            const ofbx::Vec2 duv20 = uv2 - uv0;

            const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
            ofbx::Vec3 tangent;
            tangent.x     = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
            tangent.y     = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
            tangent.z     = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
            const float l = 1 / sqrt(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
            tangent.x *= l;
            tangent.y *= l;
            tangent.z *= l;
            out[i + 0] = tangent;
            out[i + 1] = tangent;
            out[i + 2] = tangent;
        }
    }

    glm::vec2 ToNekoEngineVector(const ofbx::Vec2& vec)
    {
        return glm::vec2(float(vec.x), float(vec.y));
    }

    glm::vec3 ToNekoEngineVector(const ofbx::Vec3& vec)
    {
        return glm::vec3(float(vec.x), float(vec.y), float(vec.z));
    }

    glm::vec4 ToNekoEngineVector(const ofbx::Vec4& vec)
    {
        return glm::vec4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
    }

    glm::vec4 ToNekoEngineVector(const ofbx::Color& vec)
    {
        return glm::vec4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
    }

    glm::quat ToNekoEngineQuat(const ofbx::Quat& quat)
    {
        return glm::quat(float(quat.x), float(quat.y), float(quat.z), float(quat.w));
    }

    bool IsMeshInvalid(const ofbx::Mesh* aMesh)
    {
        return aMesh->getGeometry()->getVertexCount() == 0;
    }

    Texture2D* LoadTexture(const ofbx::Material* material, ofbx::Texture::TextureType type)
    {
        const ofbx::Texture* ofbxTexture = material->getTexture(type);
        Texture2D* texture2D   = nullptr;
        if(ofbxTexture)
        {
            std::string stringFilepath;
            ofbx::DataView filename = ofbxTexture->getRelativeFileName();
            if(filename == "")
                filename = ofbxTexture->getFileName();

            char filePath[MAX_PATH_LENGTH];
            filename.toString(filePath);

            stringFilepath = std::string(filePath);
            stringFilepath = modelDirectory + "/" + StringUtility::BackSlashesToSlashes(stringFilepath);

            bool fileFound = false;

            fileFound = FileSystem::FileExists(stringFilepath);

            if(!fileFound)
            {
                stringFilepath = StringUtility::GetFileName(stringFilepath);
                stringFilepath = modelDirectory + "/" + stringFilepath;
                fileFound      = FileSystem::FileExists(stringFilepath);
            }

            if(!fileFound)
            {
                stringFilepath = StringUtility::GetFileName(stringFilepath);
                stringFilepath = modelDirectory + "/textures/" + stringFilepath;
                fileFound      = FileSystem::FileExists(stringFilepath);
            }

            if(fileFound)
            {
                texture2D = GET_RHI_FACTORY()->CreateTexture2DFromFile(stringFilepath, stringFilepath);
            }
        }

        return texture2D;
    }

    Transform GetTransform(const ofbx::Object* mesh)
    {
        auto transform = Transform();

        ofbx::DVec3 p = mesh->getLocalTranslation();

        glm::vec3 pos = (glm::vec3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)));
        transform.localPosition = FixOrientation(pos);

        ofbx::DVec3 r  = mesh->getLocalRotation();
        glm::vec3 rot = FixOrientation(glm::vec3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
        transform.localOrientation = glm::quat(glm::vec3(rot.x, rot.y, rot.z));

        ofbx::DVec3 s  = mesh->getLocalScaling();
        glm::vec3 scl = glm::vec3(static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z));
        transform.localScale = scl;

        if(mesh->getParent())
        {
            transform.worldMatrix = GetTransform(mesh->getParent()).GetWorldMatrix();
        }
        else
            transform.worldMatrix = glm::mat4(1.0f);

        return transform;
    }

    SharedPtr<Material> LoadMaterial(const ofbx::Material* material, bool animated)
    {
        // auto shader = animated ? gEngine->GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader") : gEngine->GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        auto shader = GET_SHADER_LIB()->GetResource("ForwardPBR");

        SharedPtr<Material> pbrMaterial = MakeShared<Material>(shader);

        PBRMaterialTextures textures;
        MaterialProperties properties;

        properties.albedoColour = ToNekoEngineVector(material->getDiffuseColor());
        properties.metallic     = material->getSpecularColor().r;

        float roughness      = 1.0f - sqrt(float(material->getShininess()) / 100.0f);
        properties.roughness = roughness;
        properties.roughness = roughness;

        textures.albedo = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::DIFFUSE));
        textures.normal = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::NORMAL));
        // textures.metallic = LoadTexture(material, ofbx::Texture::TextureType::REFLECTION);
        textures.metallic  = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::SPECULAR));
        textures.roughness = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::SHININESS));
        textures.emissive  = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::EMISSIVE));
        textures.ao        = SharedPtr<Texture2D>(LoadTexture(material, ofbx::Texture::TextureType::AMBIENT));

        if(!textures.albedo)
            properties.albedoMapFactor = 0.0f;
        if(!textures.normal)
            properties.normalMapFactor = 0.0f;
        if(!textures.metallic)
            properties.metallicMapFactor = 0.0f;
        if(!textures.roughness)
            properties.roughnessMapFactor = 0.0f;
        if(!textures.emissive)
            properties.emissiveMapFactor = 0.0f;
        if(!textures.ao)
            properties.occlusionMapFactor = 0.0f;

        pbrMaterial->SetTextures(textures);
        pbrMaterial->SetMaterialProperites(properties);

        return pbrMaterial;
    }

    SharedPtr<Mesh> LoadMesh(const ofbx::Mesh* fbxMesh, int32_t triangleStart, int32_t triangleEnd)
    {
        const int32_t firstVertexOffset = triangleStart * 3;
        const int32_t lastVertexOffset  = triangleEnd * 3;
        const int vertexCount           = lastVertexOffset - firstVertexOffset + 3;

        auto geom                  = fbxMesh->getGeometry();
        auto numIndices            = geom->getIndexCount();
        int vertex_count           = geom->getVertexCount();
        const ofbx::Vec3* vertices = geom->getVertices();
        const ofbx::Vec3* normals  = geom->getNormals();
        const ofbx::Vec3* tangents = geom->getTangents();
        // const ofbx::Vec3* bitangents   = geom->getBitangents();
        const ofbx::Vec4* colours = geom->getColors();
        const ofbx::Vec2* uvs     = geom->getUVs();
        const int* materials      = geom->getMaterials();
        std::vector<Vertex> tempvertices(vertex_count);
        std::vector<uint32_t> indicesArray(numIndices);
        ofbx::Vec3* generatedTangents = nullptr;

        int indexCount = 0;
        auto indices   = geom->getFaceIndices();

        auto transform = GetTransform(fbxMesh);

        for(int i = 0; i < vertexCount; i++)
        {
            ofbx::Vec3 cp = vertices[i + firstVertexOffset];

            auto& vertex    = tempvertices[i];
            vertex.position = transform.GetWorldMatrix() * glm::vec4(float(cp.x), float(cp.y), float(cp.z), 1.0f);
            FixOrientation(vertex.position);

            if(normals)
                vertex.normal = transform.GetWorldMatrix() * glm::normalize(glm::vec4(float(normals[i + firstVertexOffset].x), float(normals[i + firstVertexOffset].y), float(normals[i + firstVertexOffset].z), 1.0f));
            if(uvs)
                vertex.texCoords = glm::vec2(float(uvs[i + firstVertexOffset].x), 1.0f - float(uvs[i + firstVertexOffset].y));
            if(colours)
                vertex.color = glm::vec4(float(colours[i + firstVertexOffset].x), float(colours[i + firstVertexOffset].y), float(colours[i + firstVertexOffset].z), float(colours[i + firstVertexOffset].w));

            FixOrientation(vertex.normal);
            FixOrientation(vertex.tangent);
        }

        for(int i = 0; i < vertexCount; i++)
        {
            indexCount++;

            int index       = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];
            indicesArray[i] = i; // index;
        }

        const ofbx::Material* material = nullptr;
        if(fbxMesh->getMaterialCount() > 0)
        {
            if(geom->getMaterials())
                material = fbxMesh->getMaterial(geom->getMaterials()[triangleStart]);
            else
                material = fbxMesh->getMaterial(0);
        }

        SharedPtr<Material> pbrMaterial;
        if(material)
        {
            pbrMaterial = LoadMaterial(material, false);
        }

        auto mesh = MakeShared<Mesh>(indicesArray, tempvertices);
        mesh->SetName(fbxMesh->name);
        if(material)
            mesh->SetMaterial(pbrMaterial);

        mesh->Mesh::GenerateTangentsAndBitangents(tempvertices.data(), uint32_t(vertexCount), indicesArray.data(), uint32_t(indicesArray.size()));

        return mesh;
    }

    void Model::LoadFBX(const String& _path)
    {
        String errorInfo;
        String path = _path;
        path = StringUtility::BackSlashesToSlashes(path);
        modelDirectory = path.substr(0, path.find_last_of('/'));

        std::string name = modelDirectory.substr(modelDirectory.find_last_of('/') + 1);

        std::string ext = StringUtility::GetFilePathExtension(path);
        int64_t size    = FileSystem::GetFileSize(path);
        auto data       = FileSystem::ReadFile(path);

        if(data == nullptr)
        {
            LOG("Failed to load fbx file");
            return;
        }
        const bool ignoreGeometry = false;
        const uint64_t flags      = ignoreGeometry ? (uint64_t)ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t)ofbx::LoadFlags::TRIANGULATE;

        ofbx::IScene* scene = ofbx::load(data, uint32_t(size), flags);

        errorInfo = ofbx::getError();

        if(!errorInfo.empty() || !scene)
        {
            LOG(errorInfo.c_str());
        }

        const ofbx::GlobalSettings* settings = scene->getGlobalSettings();
        switch(settings->UpAxis)
        {
            case ofbx::UpVector_AxisX:
                orientation = Orientation::X_UP;
                break;
            case ofbx::UpVector_AxisY:
                orientation = Orientation::Y_UP;
                break;
            case ofbx::UpVector_AxisZ:
                orientation = Orientation::Z_UP;
                break;
        }

        int meshCount = scene->getMeshCount();

        //TODO: Use Job System

        for(int i = 0; i < meshCount; ++i)
        {
            const auto* fbxMesh = (const ofbx::Mesh*)scene->getMesh(i);
            const auto geometry = fbxMesh->getGeometry();
            const auto trianglesCount = geometry->getVertexCount() / 3;

            if(IsMeshInvalid(fbxMesh))
                continue;

            if (fbxMesh->getMaterialCount() < 2 || !geometry->getMaterials())
            {
                meshes.push_back(LoadMesh(fbxMesh, 0, trianglesCount - 1));
            }
            else
            {
                // Create mesh for each material

                const auto materials = geometry->getMaterials();
                int32_t rangeStart = 0;
                int32_t rangeStartMaterial = materials[rangeStart];
                for (int32_t triangleIndex = 1; triangleIndex < trianglesCount; triangleIndex++)
                {
                    if (rangeStartMaterial != materials[triangleIndex])
                    {
                        meshes.push_back(LoadMesh(fbxMesh, rangeStart, triangleIndex - 1));

                        // Start a new range
                        rangeStart = triangleIndex;
                        rangeStartMaterial = materials[triangleIndex];
                    }
                }
                meshes.push_back(LoadMesh(fbxMesh, rangeStart, trianglesCount - 1));
            }
        }
    }
    
    void Model::LoadOBJ(const String& _path)
    {

    }

    void Model::LoadGLTF(const String& _path)
    {

    }
} // namespace NekoEngine
