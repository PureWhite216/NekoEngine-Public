#pragma once

#include "Core.h"
#include "RHI/Texture.h"
#include "GlmSerizlization.h"
#include "File/VirtualFileSystem.h"
#include <cereal/cereal.hpp>


namespace NekoEngine
{
    class DescriptorSet;

    const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
    const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
    const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

    struct MaterialProperties
    {
        Color albedoColour = Color(1.0f, 1.0f, 1.0f, 1.0f);
        float roughness = 0.7f;
        float metallic = 0.7f;
        float reflectance = 0.3f;
        float emissive = 0.0f;
        float albedoMapFactor = 1.0f;
        float metallicMapFactor = 1.0f;
        float roughnessMapFactor = 1.0f;
        float normalMapFactor = 1.0f;
        float emissiveMapFactor = 1.0f;
        float occlusionMapFactor = 1.0f;
        float alphaCutoff = 0.4f;
        float workflow = PBR_WORKFLOW_SEPARATE_TEXTURES;
        float padding = 0.0f;
    };

    struct PBRMaterialTextures
    {
        SharedPtr<Texture2D> albedo;
        SharedPtr<Texture2D> normal;
        SharedPtr<Texture2D> metallic;
        SharedPtr<Texture2D> roughness;
        SharedPtr<Texture2D> ao;
        SharedPtr<Texture2D> emissive;
    };

    class Material
    {
    public:
        enum class RenderFlags : uint32_t
        {
            NONE = 0,
            DEPTHTEST = BIT(0),
            WIREFRAME = BIT(1),
            FORWARDRENDER = BIT(2),
            DEFERREDRENDER = BIT(3),
            NOSHADOW = BIT(4),
            TWOSIDED = BIT(5),
            ALPHABLEND = BIT(6)
        };
    protected:
        String name;
        SharedPtr<Shader> shader;
        DescriptorSet* descriptorSet;
        MaterialProperties* materialProperties;
        PBRMaterialTextures pbrMaterialTextures;
        uint32_t materialBufferSize;
        uint32_t flags;
        bool isTextureUpdated = false;

        static SharedPtr<Texture2D> defaultTexture;
    public:
        Material(SharedPtr<Shader> &shader, const MaterialProperties &properties = MaterialProperties(),
                 const PBRMaterialTextures &textures = PBRMaterialTextures());

        Material();

        virtual ~Material();

        void Bind();

        void LoadMaterial(const String &name, const String &filePath);

        void CreateDescriptorSet(int layoutID, bool pbr = true);

        void UpdateMaterialPropertiesData();

        void UpdateDescriptorSet();

        void LoadPBRMaterial(const std::string &_name, const std::string &path,
                             const std::string &extension = ".png"); // TODO : Texture Parameters

        template <typename Archive>
        void save(Archive &archive) const
        {

        }

        template <typename Archive>
        void load(Archive &archive)
        {

        }

        void SetName(const std::string &_name)
        { name = _name; }

        void SetAlbedoTexture(const std::string &path);

        void SetNormalTexture(const std::string &path);

        void SetRoughnessTexture(const std::string &path);

        void SetMetallicTexture(const std::string &path);

        void SetAOTexture(const std::string &path);

        void SetEmissiveTexture(const std::string &path);

        void SetShader(SharedPtr<Shader> &_shader)
        {
            shader = _shader;
            isTextureUpdated = true; // TODO check
        }

        void SetShader(const std::string &filePath);

        void SetTexturesUpdated(bool updated)
        { isTextureUpdated = updated; }

        void SetTextures(const PBRMaterialTextures &textures);

        void SetMaterialProperites(const MaterialProperties &properties);

        void SetFlag(RenderFlags flag, bool value = true)
        {
            if(value)
                flags |= (uint32_t) flag;
            else
                flags &= ~(uint32_t) flag;
        };


        bool IsTexturesUpdated()
        { return isTextureUpdated; }

        PBRMaterialTextures &GetTextures()
        { return pbrMaterialTextures; }

        SharedPtr<Shader> GetShader() const
        { return shader; }

        DescriptorSet* GetDescriptorSet() const
        { return descriptorSet; }

        const std::string &GetName() const
        { return name; }

        MaterialProperties* GetProperties() const
        { return materialProperties; }

        uint32_t GetFlags() const
        { return flags; };

        bool GetFlag(RenderFlags flag) const
        { return (uint32_t) flag & flags; };

        static void InitDefaultTexture();

        static void ReleaseDefaultTexture();

        static SharedPtr<Texture2D> GetDefaultTexture()
        { return defaultTexture; }
    };

} // NekoEngine

