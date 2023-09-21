#include "RenderPass.h"
#include "Hash.h"
#include "Engine.h"
namespace NekoEngine
{
    struct RenderPassAsset
    {
        SharedPtr<RenderPass> renderpass;
        float timeSinceLastAccessed;
    };
    static std::unordered_map<uint64_t, RenderPassAsset> m_RenderPassCache;
    static const float m_CacheLifeTime = 0.1f;


    SharedPtr<RenderPass> RenderPass::Get(const RenderPassDesc& renderPassDesc)
    {
        uint64_t hash = 0;
        HashCombine(hash, renderPassDesc.attachmentCount, renderPassDesc.clear);

        for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
        {
            HashCombine(hash, renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i], renderPassDesc.cubeMapIndex, renderPassDesc.mipIndex);

            if(renderPassDesc.attachments[i])
                HashCombine(hash, renderPassDesc.attachments[i]->GetUUID());
        }

        auto found = m_RenderPassCache.find(hash);
        if(found != m_RenderPassCache.end() && found->second.renderpass)
        {
            found->second.timeSinceLastAccessed = (float)gEngine->GetTimeStep()->GetElapsedSeconds();
            return found->second.renderpass;
        }

        auto renderPass         = SharedPtr<RenderPass>(GET_RHI_FACTORY()->CreateRenderPass(renderPassDesc));
        m_RenderPassCache[hash] = { renderPass, (float)gEngine->GetTimeStep()->GetElapsedSeconds() };
        return renderPass;
    }

    void RenderPass::ClearCache()
    {
        m_RenderPassCache.clear();
    }

    void RenderPass::DeleteUnusedCache()
    {
        static std::size_t keysToDelete[256];
        std::size_t keysToDeleteCount = 0;

        for(auto&& [key, value] : m_RenderPassCache)
        {
            if(value.renderpass && value.renderpass.use_count() && value.renderpass.use_count() == 1 && (gEngine->GetTimeStep()->GetElapsedSeconds() - value.timeSinceLastAccessed) > m_CacheLifeTime)
            {
                keysToDelete[keysToDeleteCount] = key;
                keysToDeleteCount++;
            }

            if(keysToDeleteCount >= 256)
                break;
        }

        for(std::size_t i = 0; i < keysToDeleteCount; i++)
        {
            m_RenderPassCache[keysToDelete[i]].renderpass = nullptr;
            m_RenderPassCache.erase(keysToDelete[i]);
        }
    }
} // NekoEngine