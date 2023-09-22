#include "Vk.h"
#include "RHI/Shader.h"

namespace spirv_cross
{
    struct SPIRType;
}

namespace NekoEngine
{

    VkFormat GetVulkanFormat(const spirv_cross::SPIRType& type);
    uint32_t GetStrideFromVulkanFormat(VkFormat format);

    class VulkanShader : public Shader
    {
    private:
        VkShaderModule handle;
        VkPipelineShaderStageCreateInfo* shaderStageInfo = nullptr;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        ArrayList<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        ArrayList<VkDescriptorSetLayout> descriptorSetLayouts;


    public:
        VulkanShader(const std::string& filePath);
        VulkanShader(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize);
        VulkanShader(const uint32_t* compData, uint32_t compDataSize);
        ~VulkanShader();

        bool Init();
        void Unload();

        VkPipelineShaderStageCreateInfo* GetShaderStages() const;
        uint32_t GetStageCount() const;

        void Bind() const override {};
        void UnBind() const override {};

        void CreatePipelineLayout();
        void ParseShaderFile(const std::vector<std::string>& lines, std::map<ShaderType, std::string>* shaders);
        void BindPushConstants(CommandBuffer* commandBuffer, Pipeline* pipeline) override;

        ShaderDataType SPIRVTypeToDataType(const spirv_cross::SPIRType type);

        ArrayList<PushConstant>& GetPushConstants() override { return pushConstants; };
        VkShaderModule& GetHandle() { return handle; };
        VkPipelineLayout& GetPipelineLayout() { return pipelineLayout; };

//        const ArrayList<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return descriptorSetLayouts; }
        const ArrayList<DescriptorLayoutInfo>& GetDescriptorLayoutInfos() const { return descriptorLayoutInfos; }
        VkDescriptorSetLayout* GetDescriptorLayout(int id)
        {
            return &descriptorSetLayouts[id];
        };

        const std::vector<VkDescriptorSetLayout>& GetDescriptorLayouts() const { return descriptorSetLayouts; }


        DescriptorSetInfo GetDescriptorInfo(uint32_t index) override
        {
            if(descriptorSetInfos.find(index) != descriptorSetInfos.end())
            {
                return descriptorSetInfos.at(index);
            }

            LOG_FORMAT("DescriptorDesc not found. Index = %d", index);
            return DescriptorSetInfo();
        }

        bool IsCompute()
        {
            if(shaderTypes.size() > 0)
                return shaderTypes[0] == ShaderType::COMPUTE;

            return false;
        }

        const uint32_t GetVertexInputStride() const { return vertexInputStride; }
        const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescription() const { return vertexInputAttributeDescriptions; }
    private:
        void LoadFromData(const uint32_t* data, uint32_t size, ShaderType type, int currentShaderStage);
    };

} // NekoEngine

