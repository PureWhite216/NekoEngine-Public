#include "ImageLoader.h"
#include "VirtualFileSystem.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
namespace NekoEngine
{
    namespace ImageLoader
    {
        uint8_t* LoadImageFromFile(const char* filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
        {
            std::string filePath = std::string(filename);
            std::string physicalPath;
            if(!VirtualFileSystem::ResolvePhysicalPath(filePath, physicalPath))
                return nullptr;

            filename = physicalPath.c_str();

            int texWidth = 0, texHeight = 0, texChannels = 0;
            stbi_uc* pixels   = nullptr;
            int sizeOfChannel = 8;
            if(stbi_is_hdr(filename))
            {
                sizeOfChannel = 32;
                pixels        = (uint8_t*)stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

                if(isHDR)
                    *isHDR = true;
            }
            else
            {
                pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

                if(isHDR)
                    *isHDR = false;
            }

            if(!pixels)
            {
                LOG("Could not load image!");
                // Return magenta checkerboad image

                texChannels = 4;

                if(width)
                    *width = 2;
                if(height)
                    *height = 2;
                if(bits)
                    *bits = texChannels * sizeOfChannel;

                const int32_t size = (*width) * (*height) * texChannels;
                uint8_t* data      = new uint8_t[size];

                uint8_t datatwo[16] = {
                        255, 0, 255, 255,
                        0, 0, 0, 255,
                        0, 0, 0, 255,
                        255, 0, 255, 255
                };

                memcpy(data, datatwo, size);

                return data;
            }

            // TODO support different texChannels
            if(texChannels != 4)
                texChannels = 4;

            if(width)
                *width = texWidth;
            if(height)
                *height = texHeight;
            if(bits)
                *bits = texChannels * sizeOfChannel; // texChannels;	  //32 bits for 4 bytes r g b a

            const int32_t size = texWidth * texHeight * texChannels * sizeOfChannel / 8;
            uint8_t* result    = new uint8_t[size];
            memcpy(result, pixels, size);

            stbi_image_free(pixels);
            return result;
        }

        uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
        {
            return LoadImageFromFile(filename.c_str(), width, height, bits, isHDR, srgb, flipY);
        }
    }
} // NekoEngine