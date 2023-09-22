#include "VulkanShader.h"
#include "StringUtility.h"
#include "VulkanUtility.h"
#include "File/VirtualFileSystem.h"
#include "File/FileSystem.h"
#include "Hash.h"
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "spirv_cross/spirv_cross.hpp"
#include "VulkanInitializer.h"


namespace NekoEngine
{
    VkFormat GetVulkanFormat(const spirv_cross::SPIRType &type)
    {
        using namespace spirv_cross;
        if(type.basetype == SPIRType::Struct || type.basetype == SPIRType::Sampler)
        {
            LOG("Tried to convert a structure or SPIR sampler into a VkFormat enum value!");
            return VK_FORMAT_UNDEFINED;
        }
        else if(type.basetype == SPIRType::Image || type.basetype == SPIRType::SampledImage)
        {
            switch(type.image.format)
            {
                case spv::ImageFormatR8:
                    return VK_FORMAT_R8_UNORM;
                case spv::ImageFormatR8Snorm:
                    return VK_FORMAT_R8_SNORM;
                case spv::ImageFormatR8ui:
                    return VK_FORMAT_R8_UINT;
                case spv::ImageFormatR8i:
                    return VK_FORMAT_R8_SINT;
                case spv::ImageFormatRg8:
                    return VK_FORMAT_R8G8_UNORM;
                case spv::ImageFormatRg8Snorm:
                    return VK_FORMAT_R8G8_SNORM;
                case spv::ImageFormatRg8ui:
                    return VK_FORMAT_R8G8_UINT;
                case spv::ImageFormatRg8i:
                    return VK_FORMAT_R8G8_SINT;
                case spv::ImageFormatRgba8i:
                    return VK_FORMAT_R8G8B8A8_SINT;
                case spv::ImageFormatRgba8ui:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case spv::ImageFormatRgba8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case spv::ImageFormatRgba8Snorm:
                    return VK_FORMAT_R8G8B8A8_SNORM;
                case spv::ImageFormatR32i:
                    return VK_FORMAT_R32_SINT;
                case spv::ImageFormatR32ui:
                    return VK_FORMAT_R32_UINT;
                case spv::ImageFormatRg32i:
                    return VK_FORMAT_R32G32_SINT;
                case spv::ImageFormatRg32ui:
                    return VK_FORMAT_R32G32_UINT;
                case spv::ImageFormatRgba32f:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case spv::ImageFormatRgba16f:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case spv::ImageFormatR32f:
                    return VK_FORMAT_R32_SFLOAT;
                case spv::ImageFormatRg32f:
                    return VK_FORMAT_R32G32_SFLOAT;
                case spv::ImageFormatR16f:
                    return VK_FORMAT_R16_SFLOAT;
                case spv::ImageFormatRgba32i:
                    return VK_FORMAT_R32G32B32A32_SINT;
                case spv::ImageFormatRgba32ui:
                    return VK_FORMAT_R32G32B32A32_UINT;
                default:
                    LOG("Failed to convert an image format to a VkFormat enum.");
                    return VK_FORMAT_UNDEFINED;
            }
        }
        else if(type.vecsize == 1)
        {
            if(type.width == 8)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 16)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 32)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 64)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                LOG("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                return VK_FORMAT_UNDEFINED;
            }
        }
        else if(type.vecsize == 2)
        {
            if(type.width == 8)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 16)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 32)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 64)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                LOG("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                return VK_FORMAT_UNDEFINED;
            }
        }
        else if(type.vecsize == 3)
        {
            if(type.width == 8)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8B8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8B8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 16)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16B16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16B16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16B16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 32)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32B32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32B32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 64)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64B64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64B64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64B64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                LOG("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                return VK_FORMAT_UNDEFINED;
            }
        }
        else if(type.vecsize == 4)
        {
            if(type.width == 8)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8B8A8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8B8A8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 16)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16B16A16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16B16A16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16B16A16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 32)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32B32A32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32B32A32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.width == 64)
            {
                switch(type.basetype)
                {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64B64A64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64B64A64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64B64A64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                LOG("Invalid type width for conversion to a VkFormat enum");
                return VK_FORMAT_UNDEFINED;
            }
        }
        else
        {
            LOG("Vector size in vertex input attributes isn't explicitly supported for parsing from SPIRType->VkFormat");
            return VK_FORMAT_UNDEFINED;
        }
    }

    VulkanShader::VulkanShader(const String &_filePath)
    {
        name = StringUtility::GetFileName(_filePath);
        filePath = StringUtility::GetFileLocation(_filePath);
        source = VirtualFileSystem::ReadTextFile(_filePath);

        LOG_FORMAT("Loading Shader %s, from %s [Physical: %s]", name.c_str(), filePath.c_str(), _filePath.c_str());

        if(source.empty())
        {
            LOG_FORMAT("Failed to read shader source from file: %s", _filePath.c_str());
            return;
        }
        Init();
    }

    bool VulkanShader::Init()
    {
        stageCount = 0;
        Map<ShaderType, String> shaderFiles;
        ArrayList<String> lines = StringUtility::GetLines(source);
        ParseShaderFile(lines, &shaderFiles);

        for(auto &file: shaderFiles)
        {
            shaderTypes.push_back(file.first);
            stageCount++;
        }

        shaderStageInfo = new VkPipelineShaderStageCreateInfo[stageCount];

        for(int i = 0; i < stageCount; i++)
        {
            shaderStageInfo[i] = VkPipelineShaderStageCreateInfo();
        }
        LOG_FORMAT("Loading Shader %s.", name.c_str());

        uint32_t currentShaderStage = 0;
        HashCombine(hash, name);

        for(auto &file: shaderFiles)
        {
            HashCombine(hash, filePath + file.second);
            uint32_t fileSize = uint32_t(FileSystem::GetFileSize(filePath + file.second));
            uint32_t* source = reinterpret_cast<uint32_t*>(FileSystem::ReadFile(filePath + file.second));
            if(source)
            {
                LoadFromData(source, fileSize, file.first, currentShaderStage);
                currentShaderStage++;
                delete[] source;
            }
        }

        if(shaderFiles.empty())
        {
            LOG_FORMAT("Failed to load shader %s", name.c_str());
        }

        CreatePipelineLayout();
        return true;
    }

    void VulkanShader::BindPushConstants(CommandBuffer* commandBuffer, Pipeline* pipeline)
    {
        uint32_t index = 0;
        for(auto& pc : pushConstants)
        {
            vkCmdPushConstants(dynamic_cast<VulkanCommandBuffer*>(commandBuffer)->GetHandle(), dynamic_cast<VulkanPipeline*>(pipeline)->GetPipelineLayout(), VulkanUtility::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
        }
    }

    void VulkanShader::CreatePipelineLayout()
    {
        ArrayList<ArrayList<DescriptorLayoutInfo>> layouts;

        for(auto &descriptorLayout: descriptorLayoutInfos)
        {
            while(layouts.size() < descriptorLayout.setID + 1)
            {
                layouts.emplace_back();
            }

            layouts[descriptorLayout.setID].push_back(descriptorLayout);
        }

        for(auto &l: layouts)
        {
            std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
            std::vector<VkDescriptorBindingFlags> layoutBindingFlags;
            setLayoutBindings.reserve(l.size());
            layoutBindingFlags.reserve(l.size());

            for(uint32_t i = 0; i < l.size(); i++)
            {
                auto &info = l[i];

                VkDescriptorSetLayoutBinding setLayoutBinding = {};
                setLayoutBinding.descriptorType = VulkanUtility::DescriptorTypeToVK(info.type);
                setLayoutBinding.stageFlags = VulkanUtility::ShaderTypeToVK(info.stage);
                setLayoutBinding.binding = info.binding;
                setLayoutBinding.descriptorCount = info.count;

                bool isArray = info.count > 1;
                layoutBindingFlags.push_back(isArray ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
                setLayoutBindings.push_back(setLayoutBinding);
            }

            VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
            flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            flagsInfo.pNext = nullptr;
            flagsInfo.bindingCount = static_cast<uint32_t>(layoutBindingFlags.size());
            flagsInfo.pBindingFlags = layoutBindingFlags.data();

            // Pipeline layout
            VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
            setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
            setLayoutCreateInfo.pBindings = setLayoutBindings.data();
            setLayoutCreateInfo.pNext = &flagsInfo;

            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(GET_DEVICE(), &setLayoutCreateInfo, VK_NULL_HANDLE, &layout);

            descriptorSetLayouts.push_back(layout);
        }

        const auto& pushConsts = GetPushConstants();
        std::vector<VkPushConstantRange> pushConstantRanges;

        for(auto& pushConst : pushConsts)
        {
            pushConstantRanges.push_back(VKInitialisers::PushConstantRange(VulkanUtility::ShaderTypeToVK(pushConst.shaderStage), pushConst.size, pushConst.offset));
        }

        auto& tDescriptorSetLayouts = GetDescriptorLayouts();

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount             = static_cast<uint32_t>(tDescriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts                = tDescriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount     = uint32_t(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges        = pushConstantRanges.data();

        VK_CHECK_RESULT(vkCreatePipelineLayout(GET_DEVICE(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout), "Failed to create pipeline layout");

    }

    void VulkanShader::ParseShaderFile(const std::vector<std::string> &lines, std::map<ShaderType, String>* shaders)
    {
        for(uint32_t i = 0; i < lines.size(); i++)
        {
            String line = String(lines[i]);
            line = StringUtility::StringReplace(line, '\t');
            ShaderType type;
            if(StringUtility::StartsWith(line, "#shader"))
            {
                if(StringUtility::StringContains(line, "vertex"))
                {
                    type = ShaderType::VERTEX;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "geometry"))
                {
                    type = ShaderType::GEOMETRY;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "fragment"))
                {
                    type = ShaderType::FRAGMENT;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "tess_cont"))
                {
                    type = ShaderType::TESSELLATION_CONTROL;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "tess_eval"))
                {
                    type = ShaderType::TESSELLATION_EVALUATION;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "compute"))
                {
                    type = ShaderType::COMPUTE;
                    auto it = shaders->begin();
                    shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
                }
                else if(StringUtility::StringContains(line, "end"))
                {
                    type = ShaderType::UNKNOWN;
                }
            }
        }
    }

    uint32_t GetStrideFromVulkanFormat(VkFormat format)
    {
        switch(format)
        {
            case VK_FORMAT_R8_SINT:
                return sizeof(int);
            case VK_FORMAT_R32_SFLOAT:
                return sizeof(float);
            case VK_FORMAT_R32G32_SFLOAT:
                return sizeof(glm::vec2);
            case VK_FORMAT_R32G32B32_SFLOAT:
                return sizeof(glm::vec3);
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return sizeof(glm::vec4);
            case VK_FORMAT_R32G32_SINT:
                return sizeof(glm::ivec2);
            case VK_FORMAT_R32G32B32_SINT:
                return sizeof(glm::ivec3);
            case VK_FORMAT_R32G32B32A32_SINT:
                return sizeof(glm::ivec4);
            case VK_FORMAT_R32G32_UINT:
                return sizeof(glm::uvec2);
            case VK_FORMAT_R32G32B32_UINT:
                return sizeof(glm::uvec3);
            case VK_FORMAT_R32G32B32A32_UINT:
                return sizeof(glm::uvec4);
            default:
                LOG("Unsupported Format");
                return 0;
        }

        return 0;
    }

    void VulkanShader::LoadFromData(const uint32_t* source, uint32_t fileSize, ShaderType shaderType, int currentShaderStage)
    {
        VkShaderModuleCreateInfo shaderCreateInfo = {};
        shaderCreateInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfo.codeSize                 = fileSize;
        shaderCreateInfo.pCode                    = source;
        shaderCreateInfo.pNext                    = VK_NULL_HANDLE;

        std::vector<uint32_t> spv(source, source + fileSize / sizeof(uint32_t));

        spirv_cross::Compiler comp(std::move(spv));
        // The SPIR-V is now parsed, and we can perform reflection on it.
        spirv_cross::ShaderResources resources = comp.get_shader_resources();

        if(shaderType == ShaderType::VERTEX)
        {
            // Vertex Layout
            vertexInputStride = 0;

            for(const spirv_cross::Resource& resource : resources.stage_inputs)
            {
                const spirv_cross::SPIRType& InputType = comp.get_type(resource.type_id);

                VkVertexInputAttributeDescription Description = {};
                Description.binding                           = comp.get_decoration(resource.id, spv::DecorationBinding);
                Description.location                          = comp.get_decoration(resource.id, spv::DecorationLocation);
                Description.offset                            = vertexInputStride;
                Description.format                            = GetVulkanFormat(InputType);
                vertexInputAttributeDescriptions.push_back(Description);

                vertexInputStride += GetStrideFromVulkanFormat(Description.format);
            }
        }

        // Descriptor Layout
        for(auto& u : resources.uniform_buffers)
        {
            uint32_t set     = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
            auto& type       = comp.get_type(u.type_id);

            LOG_FORMAT("Found UBO %s at set = %d, binding = %d", u.name.c_str(), set, binding);
            descriptorLayoutInfos.push_back({ DescriptorType::UNIFORM_BUFFER, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

            auto& bufferType      = comp.get_type(u.base_type_id);
            auto bufferSize       = comp.get_declared_struct_size(bufferType);
            int memberCount       = (int)bufferType.member_types.size();
            auto& descriptorInfo  = descriptorSetInfos[set];
            auto& descriptor      = descriptorInfo.descriptors.emplace_back();
            descriptor.binding    = binding;
            descriptor.size       = (uint32_t)bufferSize;
            descriptor.name       = u.name;
            descriptor.offset     = 0;
            descriptor.shaderType = shaderType;
            descriptor.type       = DescriptorType::UNIFORM_BUFFER;
            descriptor.buffer     = nullptr;

            for(int i = 0; i < memberCount; i++)
            {
                auto type              = comp.get_type(bufferType.member_types[i]);
                const auto& memberName = comp.get_member_name(bufferType.self, i);
                auto size              = comp.get_declared_struct_member_size(bufferType, i);
                auto offset            = comp.type_struct_member_offset(bufferType, i);

                std::string uniformName = u.name + "." + memberName;

                auto& member  = descriptor.m_Members.emplace_back();
                member.name   = memberName;
                member.offset = offset;
                member.size   = (uint32_t)size;

//                (LOG_FORMAT("{0} - Size {1}, offset {2}", uniformName, size, offset));
            }
        }

        for(auto& u : resources.push_constant_buffers)
        {
            uint32_t set     = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
            uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);

            uint32_t binding3 = comp.get_decoration(u.id, spv::DecorationOffset);

            auto& type = comp.get_type(u.type_id);

            auto ranges = comp.get_active_buffer_ranges(u.id);

            uint32_t size = 0;
            for(auto& range : ranges)
            {
//                LOG_FORMAT("Accessing Member {0} offset {1}, size {2}", range.index, range.offset, range.range);
                size += uint32_t(range.range);
            }

//            LOG_FORMAT("Found Push Constant {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding, type.array.size() ? uint32_t(type.array[0]) : 1);

            pushConstants.push_back({ size, shaderType });
            pushConstants.back().data = new uint8_t[size];

            auto& bufferType = comp.get_type(u.base_type_id);
            auto bufferSize  = comp.get_declared_struct_size(bufferType);
            int memberCount  = (int)bufferType.member_types.size();

            for(int i = 0; i < memberCount; i++)
            {
                auto type               = comp.get_type(bufferType.member_types[i]);
                const auto& memberName  = comp.get_member_name(bufferType.self, i);
                auto size               = comp.get_declared_struct_member_size(bufferType, i);
                auto offset             = comp.type_struct_member_offset(bufferType, i);
                std::string uniformName = u.name + "." + memberName;
                auto& member            = pushConstants.back().m_Members.emplace_back();
                member.size             = (uint32_t)size;
                member.offset           = offset;
                member.type             = SPIRVTypeToDataType(type);
                member.fullName         = uniformName;
                member.name             = memberName;
            }
        }

        for(auto& u : resources.sampled_images)
        {
            uint32_t set         = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
            uint32_t binding     = comp.get_decoration(u.id, spv::DecorationBinding);
            auto& descriptorInfo = descriptorSetInfos[set];
            auto& descriptor     = descriptorInfo.descriptors.emplace_back();

            auto& type = comp.get_type(u.type_id);
//            (LOG_FORMAT("Found Sampled Image {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding));

            descriptorLayoutInfos.push_back({ DescriptorType::IMAGE_SAMPLER, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

            descriptor.binding      = binding;
            descriptor.textureCount = 1;
            descriptor.name         = u.name;
            descriptor.texture      = Material::GetDefaultTexture().get(); // TODO: Move
        }

        for(auto& u : resources.storage_images)
        {
            const auto& name       = u.name;
            auto& type             = comp.get_type(u.type_id);
            uint32_t binding       = comp.get_decoration(u.id, spv::DecorationBinding);
            uint32_t descriptorSet = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
            uint32_t dimension     = type.image.dim;
            uint32_t arraySize     = type.array[0];
            if(arraySize == 0)
                arraySize = 1;

            auto& descriptorInfo = descriptorSetInfos[descriptorSet];
            auto& descriptor     = descriptorInfo.descriptors.emplace_back();

//            (LOG_FORMAT("Found Storage Image {0} at set = {1}, binding = {2}", u.name.c_str(), descriptorSet, binding));

            descriptorLayoutInfos.push_back({ DescriptorType::IMAGE_STORAGE, shaderType, binding, descriptorSet, type.array.size() ? uint32_t(type.array[0]) : 1 });

            descriptor.type         = DescriptorType::IMAGE_STORAGE;
            descriptor.binding      = binding;
            descriptor.textureCount = 1;
            descriptor.name         = u.name;
            descriptor.texture      = Material::GetDefaultTexture().get(); // TODO: Move
        }

        shaderStageInfo[currentShaderStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo[currentShaderStage].stage = VulkanUtility::ShaderTypeToVK(shaderType);
        shaderStageInfo[currentShaderStage].pName = "main";
        shaderStageInfo[currentShaderStage].pNext = VK_NULL_HANDLE;

        VkResult result = vkCreateShaderModule(GET_DEVICE(), &shaderCreateInfo, nullptr, &shaderStageInfo[currentShaderStage].module);
        //TODO: Set format
        VulkanUtility::SetDebugUtilsObjectName(GET_DEVICE(), VK_OBJECT_TYPE_SHADER_MODULE, "shader", shaderStageInfo[currentShaderStage].module);

        if(result == VK_SUCCESS)
        {
            isCompiled = true;
        }

        VK_CHECK_RESULT(result, "Failed to create Shader.");
    }

    VulkanShader::VulkanShader(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData,
                               uint32_t fragDataSize)
    {
        stageCount = 0;
        pipelineLayout = VK_NULL_HANDLE;
        shaderStageInfo = nullptr;
        name = "";
        shaderTypes = {ShaderType::VERTEX, ShaderType::FRAGMENT};
        stageCount = 2;
        filePath = "Embedded";

        for(uint32_t i = 0; i < stageCount; i++)
            shaderStageInfo[i] = VkPipelineShaderStageCreateInfo();

        LoadFromData(vertData, vertDataSize, ShaderType::VERTEX, 0);
        LoadFromData(fragData, fragDataSize, ShaderType::FRAGMENT, 1);

        HashCombine(hash, name, vertData, vertData, fragData, fragDataSize);

        CreatePipelineLayout();
    }

    VulkanShader::VulkanShader(const uint32_t* compData, uint32_t compDataSize)
    {
        stageCount = 0;
        pipelineLayout = VK_NULL_HANDLE;
        shaderStageInfo = nullptr;
        name = "";
        shaderTypes = {ShaderType::COMPUTE};
        stageCount = 1;
        for(uint32_t i = 0; i < stageCount; i++)
             shaderStageInfo[i] = VkPipelineShaderStageCreateInfo();

        LoadFromData(compData, compDataSize, ShaderType::COMPUTE, 0);

        CreatePipelineLayout();
    }
    VulkanShader::~VulkanShader()
    {
        Unload();
    }


    void VulkanShader::Unload()
    {
        for(uint32_t i = 0; i < stageCount; i++)
            vkDestroyShaderModule(GET_DEVICE(), shaderStageInfo[i].module, nullptr);

        for(auto& descriptorLayout : descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(GET_DEVICE(), descriptorLayout, VK_NULL_HANDLE);

        if(pipelineLayout)
            vkDestroyPipelineLayout(GET_DEVICE(), pipelineLayout, VK_NULL_HANDLE);

        delete[] shaderStageInfo;

        for(auto& pc : pushConstants)
            delete[] pc.data;

        stageCount = 0;
    }

    ShaderDataType VulkanShader::SPIRVTypeToDataType(const spirv_cross::SPIRType type)
    {
        switch(type.basetype)
        {
            case spirv_cross::SPIRType::Boolean:
                return ShaderDataType::BOOL;
            case spirv_cross::SPIRType::Int:
                if(type.vecsize == 1)
                    return ShaderDataType::INT;
                if(type.vecsize == 2)
                    return ShaderDataType::IVEC2;
                if(type.vecsize == 3)
                    return ShaderDataType::IVEC3;
                if(type.vecsize == 4)
                    return ShaderDataType::IVEC4;

            case spirv_cross::SPIRType::UInt:
                return ShaderDataType::UINT;
            case spirv_cross::SPIRType::Float:
                if(type.columns == 3)
                    return ShaderDataType::MAT3;
                if(type.columns == 4)
                    return ShaderDataType::MAT4;

                if(type.vecsize == 1)
                    return ShaderDataType::FLOAT32;
                if(type.vecsize == 2)
                    return ShaderDataType::VEC2;
                if(type.vecsize == 3)
                    return ShaderDataType::VEC3;
                if(type.vecsize == 4)
                    return ShaderDataType::VEC4;
                break;
            case spirv_cross::SPIRType::Struct:
                return ShaderDataType::STRUCT;
        }
        LOG("Unknown spirv type!");
        return ShaderDataType::NONE;
    }

    VkPipelineShaderStageCreateInfo* VulkanShader::GetShaderStages() const
    {
        return shaderStageInfo;
    }

    uint32_t VulkanShader::GetStageCount() const
    {
        return stageCount;
    }

} // NekoEngine