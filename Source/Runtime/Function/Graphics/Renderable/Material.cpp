#include "Material.h"
#include "File/FileSystem.h"
#include "File/VirtualFileSystem.h"
#include "Engine.h"
#include "cereal/types/string.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/variant.hpp"
#include "GlmSerizlization.h"
#include "Math/Maths.h"

namespace NekoEngine
{
    SharedPtr<Texture2D> Material::defaultTexture = nullptr;

    Material::Material(SharedPtr<Shader> &_shader, const MaterialProperties &properties,
                       const PBRMaterialTextures &textures)
            : pbrMaterialTextures(textures), shader(_shader)
    {
        flags = 0;
        SetFlag(RenderFlags::DEPTHTEST);
        descriptorSet = nullptr;
        materialProperties = new MaterialProperties();
        materialBufferSize = sizeof(MaterialProperties);

        descriptorSet = nullptr;

        SetMaterialProperites(properties);
    }

    Material::Material()
            : shader(nullptr)
    {

        flags = 0;
        SetFlag(RenderFlags::DEPTHTEST);
        descriptorSet = nullptr;
        materialProperties = new MaterialProperties();
        pbrMaterialTextures.albedo = nullptr;
        materialBufferSize = sizeof(MaterialProperties);
    }

    Material::~Material()
    {
        delete descriptorSet;
        delete materialProperties;
    }

    void Material::SetTextures(const PBRMaterialTextures &textures)
    {
        pbrMaterialTextures.albedo = textures.albedo;
        pbrMaterialTextures.normal = textures.normal;
        pbrMaterialTextures.roughness = textures.roughness;
        pbrMaterialTextures.metallic = textures.metallic;
        pbrMaterialTextures.ao = textures.ao;
        pbrMaterialTextures.emissive = textures.emissive;
    }

    bool FileExists(const std::string &path)
    {


        std::string physicalPath;

        VirtualFileSystem::ResolvePhysicalPath(path, physicalPath);
        return FileSystem::FileExists(physicalPath);
    }

    void Material::LoadPBRMaterial(const std::string &_name, const std::string &path, const std::string &extension)
    {
        name = _name;
        pbrMaterialTextures = PBRMaterialTextures();
        auto params = TextureDesc(RHIFormat::R8G8B8A8_Unorm, TextureFilter::LINEAR, TextureFilter::LINEAR,
                                  TextureWrap::CLAMP_TO_EDGE);

        auto filePath = path + "/" + name + "/albedo" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.albedo = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/albedo" + extension, params));

        filePath = path + "/" + name + "/normal" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.normal = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/normal" + extension, params));

        filePath = path + "/" + name + "/roughness" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.roughness = SharedPtr<Texture2D>(
                    GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/roughness" + extension, params));

        filePath = path + "/" + name + "/metallic" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.metallic = SharedPtr<Texture2D>(
                    GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/metallic" + extension, params));

        filePath = path + "/" + name + "/ao" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.ao = SharedPtr<Texture2D>(
                    GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/ao" + extension, params));

        filePath = path + "/" + name + "/emissive" + extension;

        if(FileExists(filePath))
            pbrMaterialTextures.emissive = SharedPtr<Texture2D>(
                    GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path + "/" + name + "/emissive" + extension, params));
    }

    void Material::LoadMaterial(const std::string &_name, const std::string &path)
    {
        name = _name;
        pbrMaterialTextures = PBRMaterialTextures();
        pbrMaterialTextures.albedo = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path));
        pbrMaterialTextures.normal = nullptr;
        pbrMaterialTextures.roughness = nullptr;
        pbrMaterialTextures.metallic = nullptr;
        pbrMaterialTextures.ao = nullptr;
        pbrMaterialTextures.emissive = nullptr;
    }

    void Material::UpdateMaterialPropertiesData()
    {
        if(!descriptorSet)
            return;

        descriptorSet->SetUniformBufferData("UniformMaterialData", *&materialProperties);
        descriptorSet->Update();
    }

    void Material::SetMaterialProperites(const MaterialProperties &properties)
    {


        materialProperties->albedoColour = properties.albedoColour;
        materialProperties->metallic = properties.metallic;
        materialProperties->roughness = properties.roughness;
        materialProperties->emissive = properties.emissive;
        materialProperties->albedoMapFactor = properties.albedoMapFactor;
        materialProperties->normalMapFactor = properties.normalMapFactor;
        materialProperties->metallicMapFactor = properties.metallicMapFactor;
        materialProperties->roughnessMapFactor = properties.roughnessMapFactor;
        materialProperties->occlusionMapFactor = properties.occlusionMapFactor;
        materialProperties->emissiveMapFactor = properties.emissiveMapFactor;
        materialProperties->alphaCutoff = properties.alphaCutoff;
        materialProperties->workflow = properties.workflow;
        materialProperties->reflectance = properties.reflectance;

        UpdateMaterialPropertiesData();
    }

    void Material::UpdateDescriptorSet()
    {
        if(pbrMaterialTextures.albedo != nullptr)
        {
            descriptorSet->SetTexture("u_AlbedoMap", pbrMaterialTextures.albedo.get());
        }
        else
        {
            descriptorSet->SetTexture("u_AlbedoMap", defaultTexture.get());
            materialProperties->albedoMapFactor = 0.0f;
        }

        // if(pbr)
        {
            descriptorSet->SetTexture("u_MetallicMap", pbrMaterialTextures.metallic ? pbrMaterialTextures.metallic.get()
                                                                                    : defaultTexture.get());

            if(!pbrMaterialTextures.metallic)
                materialProperties->metallicMapFactor = 0.0f;

            descriptorSet->SetTexture("u_RoughnessMap",
                                      pbrMaterialTextures.roughness ? pbrMaterialTextures.roughness.get()
                                                                    : defaultTexture.get());

            if(!pbrMaterialTextures.roughness)
                materialProperties->roughnessMapFactor = 0.0f;

            if(pbrMaterialTextures.normal != nullptr)
            {
                descriptorSet->SetTexture("u_NormalMap", pbrMaterialTextures.normal.get());
            }
            else
            {
                descriptorSet->SetTexture("u_NormalMap", defaultTexture.get());
                materialProperties->normalMapFactor = 0.0f;
            }

            if(pbrMaterialTextures.ao != nullptr)
            {
                descriptorSet->SetTexture("u_AOMap", pbrMaterialTextures.ao.get());
            }
            else
            {
                descriptorSet->SetTexture("u_AOMap", defaultTexture.get());
                materialProperties->occlusionMapFactor = 0.0f;
            }

            if(pbrMaterialTextures.emissive != nullptr)
            {
                descriptorSet->SetTexture("u_EmissiveMap", pbrMaterialTextures.emissive.get());
            }
            else
            {
                descriptorSet->SetTexture("u_EmissiveMap", defaultTexture.get());
                materialProperties->emissiveMapFactor = 0.0f;
            }

            UpdateMaterialPropertiesData();
        }

        descriptorSet->Update();
    }

    void Material::CreateDescriptorSet(int layoutID, bool pbr)
    {


        if(descriptorSet)
            delete descriptorSet;

        if(!shader)
        {
            // If no shader then set it to the default pbr shader
            // TODO default to forward
            shader = GET_SHADER_LIB()->GetResource("ForwardPBR");
        }

        DescriptorDesc descriptorDesc;
        descriptorDesc.layoutIndex = layoutID;
        descriptorDesc.shader = shader.get();

        descriptorSet = GET_RHI_FACTORY()->CreateDescriptor(descriptorDesc);

        UpdateDescriptorSet();
    }

    void Material::Bind()
    {
        if(descriptorSet == nullptr || isTextureUpdated)
        {
            CreateDescriptorSet(1);
            SetTexturesUpdated(false);
        }

        descriptorSet->Update();

        // UpdateDescriptorSet();
    }

    void Material::SetShader(const std::string &filePath)
    {
        shader = GET_SHADER_LIB()->GetResource(filePath);
    }

    void Material::InitDefaultTexture()
    {
        uint32_t whiteTextureData = 0xffffffff;
        defaultTexture = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromSource(1, 1, &whiteTextureData));
    }

    void Material::ReleaseDefaultTexture()
    {
        defaultTexture.reset();
    }

    void Material::SetAlbedoTexture(const std::string &path)
    {


        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.albedo = tex;
            isTextureUpdated = true;
        }
    }

    void Material::SetNormalTexture(const std::string &path)
    {


        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.normal = tex;
            isTextureUpdated = true;
        }
    }

    void Material::SetRoughnessTexture(const std::string &path)
    {


        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.roughness = tex;
            isTextureUpdated = true;
        }
    }

    void Material::SetMetallicTexture(const std::string &path)
    {


        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.metallic = tex;
            isTextureUpdated = true;
        }
    }

    void Material::SetAOTexture(const std::string &path)
    {


        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.ao = tex;
            isTextureUpdated = true;
        }
    }

    void Material::SetEmissiveTexture(const std::string &path)
    {

        auto tex = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(path, path));
        if(tex)
        {
            pbrMaterialTextures.emissive = tex;
            isTextureUpdated = true;
        }
    }

//    template<typename Archive>
//    void Material::save(Archive &archive) const
//    {
//        std::string shaderPath = "";
//        std::string albedoFilePath = pbrMaterialTextures.albedo ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.albedo->GetFilepath()) : "";
//        std::string normalFilePath = pbrMaterialTextures.normal ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.normal->GetFilepath()) : "";
//        std::string metallicFilePath = pbrMaterialTextures.metallic ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.metallic->GetFilepath()) : "";
//        std::string roughnessFilePath = pbrMaterialTextures.roughness ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.roughness->GetFilepath()) : "";
//        std::string emissiveFilePath = pbrMaterialTextures.emissive ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.emissive->GetFilepath()) : "";
//        std::string aoFilePath = pbrMaterialTextures.ao ? VirtualFileSystem::AbsoulePathToVFS(
//                pbrMaterialTextures.ao->GetFilepath()) : "";
//
//        if(shader)
//        {
//            std::string path = shader->GetFilePath() + shader->GetName();
//            VirtualFileSystem::AbsoulePathToVFS(path, shaderPath);
//        }
//
//        archive(cereal::make_nvp("Albedo", albedoFilePath),
//                cereal::make_nvp("Normal", normalFilePath),
//                cereal::make_nvp("Metallic", metallicFilePath),
//                cereal::make_nvp("Roughness", roughnessFilePath),
//                cereal::make_nvp("Ao", aoFilePath),
//                cereal::make_nvp("Emissive", emissiveFilePath),
//                cereal::make_nvp("albedoColour", materialProperties->albedoColour),
//                cereal::make_nvp("roughnessValue", materialProperties->roughness),
//                cereal::make_nvp("metallicValue", materialProperties->metallic),
//                cereal::make_nvp("emissiveValue", materialProperties->emissive),
//                cereal::make_nvp("albedoMapFactor", materialProperties->albedoMapFactor),
//                cereal::make_nvp("metallicMapFactor", materialProperties->metallicMapFactor),
//                cereal::make_nvp("roughnessMapFactor", materialProperties->roughnessMapFactor),
//                cereal::make_nvp("normalMapFactor", materialProperties->normalMapFactor),
//                cereal::make_nvp("aoMapFactor", materialProperties->occlusionMapFactor),
//                cereal::make_nvp("emissiveMapFactor", materialProperties->emissiveMapFactor),
//                cereal::make_nvp("alphaCutOff", materialProperties->alphaCutoff),
//                cereal::make_nvp("workflow", materialProperties->workflow),
//                cereal::make_nvp("shader", shaderPath));
//
//        archive(cereal::make_nvp("Reflectance", materialProperties->reflectance));
//    }
//
//    template<typename Archive>
//    void Material::load(Archive &archive)
//    {
//        std::string albedoFilePath;
//        std::string normalFilePath;
//        std::string roughnessFilePath;
//        std::string metallicFilePath;
//        std::string emissiveFilePath;
//        std::string aoFilePath;
//        std::string shaderFilePath;
//
//        //TODO: delete
//        static const bool loadOldMaterial = false;
//        if(loadOldMaterial)
//        {
//            glm::vec4 roughness, metallic, emissive;
//            archive(cereal::make_nvp("Albedo", albedoFilePath),
//                    cereal::make_nvp("Normal", normalFilePath),
//                    cereal::make_nvp("Metallic", metallicFilePath),
//                    cereal::make_nvp("Roughness", roughnessFilePath),
//                    cereal::make_nvp("Ao", aoFilePath),
//                    cereal::make_nvp("Emissive", emissiveFilePath),
//                    cereal::make_nvp("albedoColour", materialProperties->albedoColour),
//                    cereal::make_nvp("roughnessColour", roughness),
//                    cereal::make_nvp("metallicColour", metallic),
//                    cereal::make_nvp("emissiveColour", emissive),
//                    cereal::make_nvp("usingAlbedoMap", materialProperties->albedoMapFactor),
//                    cereal::make_nvp("usingMetallicMap", materialProperties->metallicMapFactor),
//                    cereal::make_nvp("usingRoughnessMap", materialProperties->roughnessMapFactor),
//                    cereal::make_nvp("usingNormalMap", materialProperties->normalMapFactor),
//                    cereal::make_nvp("usingAOMap", materialProperties->occlusionMapFactor),
//                    cereal::make_nvp("usingEmissiveMap", materialProperties->emissiveMapFactor),
//                    cereal::make_nvp("workflow", materialProperties->workflow),
//                    cereal::make_nvp("shader", shaderFilePath));
//
//            materialProperties->emissive = emissive.x;
//            materialProperties->metallic = metallic.x;
//            materialProperties->roughness = roughness.x;
//        }
//        else
//        {
//            archive(cereal::make_nvp("Albedo", albedoFilePath),
//                    cereal::make_nvp("Normal", normalFilePath),
//                    cereal::make_nvp("Metallic", metallicFilePath),
//                    cereal::make_nvp("Roughness", roughnessFilePath),
//                    cereal::make_nvp("Ao", aoFilePath),
//                    cereal::make_nvp("Emissive", emissiveFilePath),
//                    cereal::make_nvp("albedoColour", materialProperties->albedoColour),
//                    cereal::make_nvp("roughnessValue", materialProperties->roughness),
//                    cereal::make_nvp("metallicValue", materialProperties->metallic),
//                    cereal::make_nvp("emissiveValue", materialProperties->emissive),
//                    cereal::make_nvp("albedoMapFactor", materialProperties->albedoMapFactor),
//                    cereal::make_nvp("metallicMapFactor", materialProperties->metallicMapFactor),
//                    cereal::make_nvp("roughnessMapFactor", materialProperties->roughnessMapFactor),
//                    cereal::make_nvp("normalMapFactor", materialProperties->normalMapFactor),
//                    cereal::make_nvp("aoMapFactor", materialProperties->occlusionMapFactor),
//                    cereal::make_nvp("emissiveMapFactor", materialProperties->emissiveMapFactor),
//                    cereal::make_nvp("alphaCutOff", materialProperties->alphaCutoff),
//                    cereal::make_nvp("workflow", materialProperties->workflow),
//                    cereal::make_nvp("shader", shaderFilePath));
//
//            archive(cereal::make_nvp("Reflectance", materialProperties->reflectance));
//        }
//
//        // if(!shaderFilePath.empty())
//        // SetShader(shaderFilePath);
//        // TODO: Support Custom Shaders;
//        shader = nullptr;
//
//        if(!albedoFilePath.empty())
//            pbrMaterialTextures.albedo = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("albedo", albedoFilePath));
//        if(!normalFilePath.empty())
//            pbrMaterialTextures.normal = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("roughness", normalFilePath));
//        if(!metallicFilePath.empty())
//            pbrMaterialTextures.metallic = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("metallic", metallicFilePath));
//        if(!roughnessFilePath.empty())
//            pbrMaterialTextures.roughness = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("roughness", roughnessFilePath));
//        if(!emissiveFilePath.empty())
//            pbrMaterialTextures.emissive = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("emissive", emissiveFilePath));
//        if(!aoFilePath.empty())
//            pbrMaterialTextures.ao = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile("ao", aoFilePath));
//    }


} // NekoEngine