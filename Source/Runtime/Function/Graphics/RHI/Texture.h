#pragma once

#include "Asset.h"
#include "Definitions.h"

namespace NekoEngine
{

    static const uint32_t BRDFTextureWidth = 256;
    static const uint32_t BRDFTextureHeight = 256;

    class Texture : public Asset
    {
    protected:
        uint32_t flags = 0;
        uint32_t bitsPerChannel = 8;
        uint32_t channelCount = 4;
        uint64_t m_UUID = 0;
        String fileName;
        uint32_t mipMapLevels = 1;
        RHIFormat rhiFormat;
        // TODO: Color Space Type
    public:
        SET_ASSET_TYPE(AssetType::Texture);

        virtual ~Texture() = default;

        virtual void* GetHandle() const
        { return (void*) this; };

        virtual void Bind(uint32_t slot = 0) const = 0;

        virtual void Unbind(uint32_t slot = 0) const = 0;

        virtual const std::string &GetName()
        { return fileName; }

        virtual const std::string &GetFilepath() const
        { return fileName; };

        inline virtual uint32_t GetWidth(uint32_t mip = 0) const
        { return 1024; }

        inline virtual uint32_t GetHeight(uint32_t mip = 0) const
        { return 1024; }

        const uint64_t GetUUID() const { return m_UUID; }

        //TODO: Need to Check
        virtual TextureType GetType() = 0;

        virtual RHIFormat GetFormat()
        { return rhiFormat; };

        virtual void GenerateMipMaps(CommandBuffer* commandBuffer = nullptr)
        {}

        virtual void* GetDescriptorInfo() const { return GetHandle(); }

        virtual void SetName(const std::string &name)
        { fileName = name; };

        virtual uint32_t GetSize() const
        { return 0; }

        virtual uint32_t GetMipMapLevels() const
        { return mipMapLevels; }

        virtual void* GetImageHandle()
        { return GetHandle(); };

        uint32_t GetBytesPerPixel() const { return (bitsPerChannel / 8) * channelCount; }
        uint32_t GetBitsPerChannel() const { return bitsPerChannel; }
        void SetBitsPerChannel(const uint32_t bits) { bitsPerChannel = bits; }
        uint32_t GetBytesPerChannel() const { return bitsPerChannel / 8; }

        static uint32_t BitsToChannelCount(uint32_t bits);

        static RHIFormat BitsToFormat(uint32_t bits);

        static uint32_t GetBitsFromFormat(RHIFormat format);

        static bool IsDepthStencilFormat(RHIFormat format)
        {
            return format == RHIFormat::D24_Unorm_S8_UInt || format == RHIFormat::D16_Unorm_S8_UInt ||
                   format == RHIFormat::D32_Float_S8_UInt;
        }

        static bool IsDepthFormat(RHIFormat format)
        {
            return format == RHIFormat::D16_Unorm || format == RHIFormat::D32_Float ||
                   format == RHIFormat::D24_Unorm_S8_UInt || format == RHIFormat::D16_Unorm_S8_UInt ||
                   format == RHIFormat::D32_Float_S8_UInt;
        }

        static bool IsStencilFormat(RHIFormat format)
        {
            return format == RHIFormat::D24_Unorm_S8_UInt || format == RHIFormat::D16_Unorm_S8_UInt ||
                   format == RHIFormat::D32_Float_S8_UInt;
        }
    };

    class Texture2D : virtual public Texture
    {
    protected:
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t* data = nullptr;

        TextureDesc params;
        TextureLoadOptions loadOptions;
    public:
        virtual void SetData(const void* pixels) = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual void Load(uint32_t _width, uint32_t _height, void* _data, TextureDesc _parameters = TextureDesc(), TextureLoadOptions _loadOptions = TextureLoadOptions()) = 0;


        inline virtual uint32_t GetWidth(uint32_t mip = 0) const
        { return width >> mip; }

        inline virtual uint32_t GetHeight(uint32_t mip = 0) const
        { return height >> mip; }
        //TODO create
    };

    class TextureCube : public Texture
    {
    protected:
        String files[MAX_MIPS];
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t* data = nullptr;
        uint32_t layersNum = 6;
        uint32_t mipsNums = 1;

        TextureDesc params;
        TextureLoadOptions loadOptions;
    };


    class TextureDepth : virtual public Texture2D
    {

    };

    class TextureDepthArray : virtual public Texture
    {
    public:
        virtual void Init()                                                  = 0;
        virtual void Resize(uint32_t width, uint32_t height, uint32_t count) = 0;
        virtual uint32_t GetCount() const = 0;
    };

} // NekoEngine
