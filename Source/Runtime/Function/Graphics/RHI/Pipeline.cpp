#include "Pipeline.h"
#include "Hash.h"
#include "Engine.h"
#include "Texture.h"
#include "Shader.h"
#include "Engine.h"

namespace NekoEngine
{
    static std::unordered_map<uint64_t, PipelineAsset> m_PipelineCache;
    static const float m_CacheLifeTime = 0.1f;

    SharedPtr<Pipeline> Pipeline::Get(const PipelineDesc& pipelineDesc)
    {
        uint64_t hash = 0;
        HashCombine(hash, pipelineDesc.shader.get(), pipelineDesc.cullMode, pipelineDesc.isDepthBiasEnabled, (uint32_t)pipelineDesc.drawType, (uint32_t)pipelineDesc.polygonMode, pipelineDesc.isTransparencyEnabled);

        for(auto texture : pipelineDesc.colourTargets)
        {
            if(texture)
            {
                HashCombine(hash, texture->GetUUID());
            }
        }

        if(pipelineDesc.depthTarget)
        {
            HashCombine(hash, pipelineDesc.depthTarget->GetUUID());
        }

        if(pipelineDesc.depthArrayTarget)
        {
            HashCombine(hash, pipelineDesc.depthArrayTarget->GetUUID());
        }

        HashCombine(hash, pipelineDesc.isClearTargets);
        HashCombine(hash, pipelineDesc.isSwapChainTarget);
        HashCombine(hash, pipelineDesc.lineWidth);
        HashCombine(hash, pipelineDesc.depthBiasConstantFactor);
        HashCombine(hash, pipelineDesc.depthBiasSlopeFactor);
        HashCombine(hash, pipelineDesc.cubeMapIndex);
        HashCombine(hash, pipelineDesc.cubeMapTarget);
        HashCombine(hash, pipelineDesc.mipIndex);

        if(pipelineDesc.isSwapChainTarget)
        {
            // Add one swapchain image to hash
            auto texture = GET_SWAP_CHAIN()->GetCurrentImage();
            if(texture)
            {
                HashCombine(hash, texture->GetUUID());
            }
        }

        auto found = m_PipelineCache.find(hash);
        if(found != m_PipelineCache.end() && found->second.pipeline)
        {
            found->second.timeSinceLastAccessed = (float)gEngine->GetTimeStep()->GetElapsedSeconds();
            return found->second.pipeline;
        }

        SharedPtr<Pipeline> pipeline = SharedPtr<Pipeline>(GET_RHI_FACTORY()->CreatePipeline(pipelineDesc));
        m_PipelineCache[hash]        = { pipeline, (float)gEngine->GetTimeStep()->GetElapsedSeconds() };
        return pipeline;
    }

    void Pipeline::ClearCache()
    {
        m_PipelineCache.clear();
    }

    void Pipeline::DeleteUnusedCache()
    {

        static std::size_t keysToDelete[256];
        std::size_t keysToDeleteCount = 0;

        for(auto&& [key, value] : m_PipelineCache)
        {
            if(value.pipeline && value.pipeline.use_count() == 1 && (gEngine->GetTimeStep()->GetElapsedSeconds() - value.timeSinceLastAccessed) > m_CacheLifeTime)
            {
                keysToDelete[keysToDeleteCount] = key;
                keysToDeleteCount++;
            }

            if(keysToDeleteCount >= 256)
                break;
        }

        for(std::size_t i = 0; i < keysToDeleteCount; i++)
        {
            m_PipelineCache[keysToDelete[i]].pipeline = nullptr;
            m_PipelineCache.erase(keysToDelete[i]);
        }
    }

    uint32_t Pipeline::GetWidth()
    {
        if(pipelineDesc.isSwapChainTarget)
        {
            return gEngine->GetSwapChain()->GetCurrentImage()->GetWidth();
        }

        if(pipelineDesc.colourTargets[0])
        {
            return pipelineDesc.mipIndex > 0 ? pipelineDesc.colourTargets[0]->GetWidth(pipelineDesc.mipIndex) : pipelineDesc.colourTargets[0]->GetWidth();
        }

        if(pipelineDesc.depthTarget)
            return pipelineDesc.depthTarget->GetWidth();

        if(pipelineDesc.depthArrayTarget)
            return pipelineDesc.depthArrayTarget->GetWidth();

        if(pipelineDesc.cubeMapTarget)
        {
            return pipelineDesc.mipIndex > 0 ? pipelineDesc.cubeMapTarget->GetWidth(pipelineDesc.mipIndex) : pipelineDesc.cubeMapTarget->GetWidth();
        }

        LOG("Invalid pipeline width");

        return 0;
    }

    uint32_t Pipeline::GetHeight()
    {
        if(pipelineDesc.isSwapChainTarget)
        {
            return gEngine->GetSwapChain()->GetCurrentImage()->GetHeight();
        }

        if(pipelineDesc.colourTargets[0])
        {
            // TODO
            return pipelineDesc.mipIndex > 0 ? pipelineDesc.colourTargets[0]->GetHeight(pipelineDesc.mipIndex) : pipelineDesc.colourTargets[0]->GetHeight();
        }

        if(pipelineDesc.depthTarget)
            return pipelineDesc.depthTarget->GetHeight();

        if(pipelineDesc.depthArrayTarget)
            return pipelineDesc.depthArrayTarget->GetHeight();

        if(pipelineDesc.cubeMapTarget)
        {
            return pipelineDesc.mipIndex > 0 ? pipelineDesc.cubeMapTarget->GetHeight(pipelineDesc.mipIndex) : pipelineDesc.cubeMapTarget->GetHeight();
        }

        LOG("Invalid pipeline height");

        return 0;
    }

} // NekoEngine