#pragma once
#include "Core.h"
#include "Definitions.h"
#include "Texture.h"
#include "RenderPass.h"
namespace NekoEngine
{
    enum class CubeFace
    {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    struct FramebufferDesc
    {
        uint32_t width;
        uint32_t height;
        uint32_t layer = 0;
        uint32_t attachmentCount;
        uint32_t msaaLevel;
        int mipIndex   = 0;
        bool screenFBO = false;
        Texture** attachments;
        TextureType* attachmentTypes;
        RenderPass* renderPass;
    };

    class Framebuffer
    {
    protected:
        uint32_t width;
        uint32_t height;
        uint32_t attachmentCount;
    public:
        virtual ~Framebuffer();

        static SharedPtr<Framebuffer> Get(const FramebufferDesc& frameBufferDesc);
//        static Framebuffer* Create(const FramebufferDesc& frameBufferDesc);

        static void ClearCache();
        static void DeleteUnusedCache();

        virtual void Validate() {};
        uint32_t GetWidth() const { return width; };
        uint32_t GetHeight() const { return height; };
//        virtual void SetClearColour(const glm::vec4& colour) = 0;
    };

} // NekoEngine

