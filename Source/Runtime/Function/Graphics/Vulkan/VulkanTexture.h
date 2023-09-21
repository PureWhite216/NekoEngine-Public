#pragma once
#include "RHI/Texture.h"
#include "Vk.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"


namespace NekoEngine
{

    VkFormat ConvertRHIFormat2VkFormat(RHIFormat format, bool isSRGB);
    VkFilter ConvertRHIFilter2VkFilter(const TextureFilter filter);

    class VulkanTexture2D : virtual public Texture2D
    {
    protected:
        VkImage image;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
        VkImageView imageView;
        VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkSampler sampler;
        VkDeviceMemory imageMemory;
        VkDescriptorImageInfo descriptor;
        HashMap<uint32_t, VkImageView> mipMaps;
        VulkanBuffer* stagingBuffer;
        VmaAllocation allocation;
        bool isDeleteImage;
    public:
        VulkanTexture2D() = default;
        VulkanTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height);
        VulkanTexture2D(TextureDesc parameters, uint32_t width, uint32_t height);
        VulkanTexture2D(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
        VulkanTexture2D(const String& name, const String& filename, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
        virtual ~VulkanTexture2D();

        void Bind(uint32_t slot = 0) const override {};
        void Unbind(uint32_t slot = 0) const override {};
        void BuildTexture();
        void TransitionImage(VkImageLayout newLayout, VulkanCommandBuffer* commandBuffer = nullptr);
        void UpdateDescriptor();
        VkImageView GetMipImageView(uint32_t mipLevel);
        void SetData(const void* pixels) override;
        void FreeResources();
        void Resize(uint32_t width, uint32_t height) override;

        bool Load();
        void Load(uint32_t _width, uint32_t _height, void* _data, TextureDesc _parameters = TextureDesc(), TextureLoadOptions _loadOptions = TextureLoadOptions()) override;


        void* GetHandle() const override { return (void*)this; }
        const VkImageView& GetImageView() const { return imageView; }
        const VkSampler& GetSampler() const { return sampler; }
        const VkDescriptorImageInfo* GetDescriptor() const { return &descriptor; }
        const VkImage& GetImage() const { return image; }
        const VkFormat& GetVkFormat() const { return vkFormat; }
        const VkImageLayout& GetImageLayout() const { return imageLayout; }
        const VmaAllocation& GetAllocation() const { return allocation; }

        void* GetDescriptorInfo() const override
        {
            return (void*)GetDescriptor();
        }


    };

    class VulkanTextureDepth : public VulkanTexture2D, public TextureDepth
    {
    public:
        VulkanTextureDepth() = default;
        VulkanTextureDepth(uint32_t _width, uint32_t _height);
        ~VulkanTextureDepth() override;

        void BuildTexture();


        virtual void* GetHandle() const override
        {
            return (void*)this;
        }

    };

    class VulkanTextureDepthArray : public TextureDepthArray, public VulkanTextureDepth
    {
    private:
        int count;
        ArrayList<VkImageView> individualImageViews;
    public:
        VulkanTextureDepthArray(uint32_t width, uint32_t height, uint32_t count);
        ~VulkanTextureDepthArray() override;

        VkImageView GetImageView(int index) const { return individualImageViews[index]; }
        uint32_t GetCount() const override { return count; }

        void BuildTexture();
    };



    // TODO: Cube Depth Array
    class VulkanTextureCube : public TextureCube
    {
    private:
        VkImage image = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkDescriptorImageInfo descriptor = {};
        VmaAllocation allocation = VK_NULL_HANDLE;
        ArrayList<VkImageView> individualImageViews;
        bool isDeleteImage = true;
    public:
        VulkanTextureCube(uint32_t size, void* data, bool hdr);
        VulkanTextureCube(const std::string& filepath);
        VulkanTextureCube(const std::string* _files);
        VulkanTextureCube(const std::string* _files, uint32_t mips, TextureDesc _params, TextureLoadOptions loadOptions);
        ~VulkanTextureCube();

        void BuildTexture();
        void FreeResources();
        void UpdateDescriptor();

        void GenerateMipMaps(CommandBuffer* commandBuffer) override;

        virtual void* GetHandle() const override
        {
            return (void*)this;
        }

        void TransitionImage(VkImageLayout newLayout, VulkanCommandBuffer* commandBuffer);

        void Bind(uint32_t slot = 0) const override {};
        void Unbind(uint32_t slot = 0) const override {};

        void Load(uint32_t mips);

        const VkImageView& GetImageView() const { return imageView; }
        //TODO: !!! deal witch mipmaps
        VkImageView GetImageView(uint32_t layer) const
        {
            return imageView;
        }

        VkImageView GetImageView(uint32_t layer, uint32_t mip) const
        {
            return imageView;
        }


        const VkSampler& GetSampler() const { return sampler; }
        const VkDescriptorImageInfo* GetDescriptor() const { return &descriptor; }
        const VkImage& GetImage() const { return image; }
        const VkFormat& GetVkFormat() const { return vkFormat; }
        const VkImageLayout& GetImageLayout() const { return imageLayout; }
        const VmaAllocation& GetAllocation() const { return allocation; }

        void* GetDescriptorInfo() const override
        {
            return (void*)GetDescriptor();
        }
    };
} // NekoEngine

