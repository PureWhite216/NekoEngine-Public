#include "AssetManager.h"
#include "Engine.h"
#include "future"
#include "File/ImageLoader.h"

namespace NekoEngine
{
    inline static std::vector<std::future<void>> m_Futures;

    static void LoadTexture2D(Texture2D* tex, const std::string& path)
    {
        uint32_t width, height, channels;
        bool hdr;
        uint32_t bits;
        uint8_t* data = ImageLoader::LoadImageFromFile(path, &width, &height, &bits, &hdr);

        TextureDesc desc;
        desc.format = bits / 4 == 8 ? RHIFormat::R8G8B8A8_Unorm : RHIFormat::R32G32B32A32_Float;

        gEngine->SubmitToMainThread([tex, path, width, height, data, desc]()
                                              { tex->Load(width, height, data, desc); });
    }

    bool TextureLibrary::Load(const std::string &filePath, SharedPtr<Texture2D> &texture)
    {
        texture = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2D({}, 1, 1));
        m_Futures.push_back(std::async(std::launch::async, &LoadTexture2D, texture.get(), filePath));
        return true;
    }

    void TextureLibrary::Destroy()
    {
        AssetManager::Destroy();
    }
    //TODO: ref?
    bool ShaderLibrary::Load(const std::string &filePath, SharedPtr<Shader> &shader)
    {
        shader = SharedPtr<Shader>(GET_RHI_FACTORY()->CreateShader(filePath));
        return true;
    }

    bool ModelLibrary::Load(const std::string &filePath, SharedPtr<Model> &model)
    {
        model = MakeShared<Model>(filePath);
        return true;
    }
} // NekoEngine