#pragma once
#include "Core.h"
#include "Definitions.h"
#include "DescriptorSet.h"
namespace NekoEngine
{
    class Shader
    {
    protected:
        String name;
        String filePath;
        String source;
        uint32_t stageCount = 0;
        ArrayList<ShaderType> shaderTypes;
        ArrayList<PushConstant> pushConstants;
        ArrayList<DescriptorLayoutInfo> descriptorLayoutInfos;
        HashMap<uint32_t, DescriptorSetInfo> descriptorSetInfos;
        bool isCompiled = false;
        uint64_t hash = 0;
        uint32_t vertexInputStride = 0;
    public:
        static const Shader* currentlyBound;
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        virtual const ArrayList<ShaderType> GetShaderTypes() const { return shaderTypes; }
        virtual const String& GetName() const { return name;}
        virtual const String& GetFilePath() const { return filePath; }
        virtual bool IsCompiled() { return isCompiled; }
        virtual ArrayList<PushConstant>& GetPushConstants() = 0;
        virtual PushConstant* GetPushConstant(uint32_t index) { return nullptr; }
        virtual void BindPushConstants(CommandBuffer* commandBuffer, Pipeline* pipeline) = 0;
        virtual DescriptorSetInfo GetDescriptorInfo(uint32_t index) { return DescriptorSetInfo(); }
        virtual uint64_t GetHash() const { return hash; }

        //TODO Create Shader
    };
}
