#include "Environment.h"
#include "StringUtility.h"
#include "Engine.h"
#include "File/VirtualFileSystem.h"
namespace NekoEngine
{
    Environment::Environment()
    {
        m_Environmnet            = nullptr;
        m_PrefilteredEnvironment = nullptr;
        m_IrradianceMap          = nullptr;
        m_IrrFactor              = 32.0f / 1024.0f;
        m_Mode                   = 1;
        m_Parameters             = { 2.0f, 0.0f, 0.0f, 1.0f };
    }

    Environment::Environment(const std::string& filepath, bool genPrefilter, bool genIrradiance)
    {
    }

    Environment::Environment(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType)
    {
        m_Width     = width;
        m_Height    = height;
        m_NumMips   = numMip;
        m_FilePath  = name;
        m_FileType  = fileType;
        m_IrrFactor = irrSizeFactor;

        Load();
    }

    void Environment::Load(const std::string& name, uint32_t numMip, uint32_t width, uint32_t height, float irrSizeFactor, const std::string& fileType)
    {
        m_Width     = width;
        m_Height    = height;
        m_NumMips   = numMip;
        m_FilePath  = name;
        m_FileType  = fileType;
        m_IrrFactor = irrSizeFactor;

        Load();
    }

    void Environment::Load()
    {
        auto* envFiles = new std::string[m_NumMips];
        auto* irrFiles = new std::string[m_NumMips];

        uint32_t currWidth  = m_Width;
        uint32_t currHeight = m_Height;

        bool failed = false;

        if(m_Mode == 0)
        {
            m_Parameters.w = m_Mode;

            if(m_FileType == ".hdr")
            {
                GET_SCENE_RENDERER()->CreateCubeMap(m_FilePath + "_Env_" + StringUtility::ToString(0) + "_" + StringUtility::ToString(currWidth) + "x" + StringUtility::ToString(currHeight) + m_FileType, m_Parameters, m_Environmnet, m_IrradianceMap);
                return;
            }
            else
            {
                for(uint32_t i = 0; i < m_NumMips; i++)
                {
                    envFiles[i] = m_FilePath + "_Env_" + StringUtility::ToString(i) + "_" + StringUtility::ToString(currWidth) + "x" + StringUtility::ToString(currHeight) + m_FileType;

                    currHeight /= 2;
                    currWidth /= 2;

                    if(currHeight < 1 || currWidth < 1)
                        break;

                    std::string newPath;
                    if(!VirtualFileSystem::ResolvePhysicalPath(envFiles[i], newPath))
                    {
                        LOG_FORMAT("Failed to load {0}", envFiles[i].c_str());
                        failed = true;
                        break;
                    }
                }

                currWidth  = (uint32_t)((float)m_Width * m_IrrFactor);
                currHeight = (uint32_t)((float)m_Height * m_IrrFactor);

                int numMipsIrr = 0;

                for(uint32_t i = 0; i < m_NumMips; i++)
                {
                    irrFiles[i] = m_FilePath + "_Irr_" + StringUtility::ToString(i) + "_" + StringUtility::ToString(currWidth) + "x" + StringUtility::ToString(currHeight) + m_FileType;

                    currHeight /= 2;
                    currWidth /= 2;

                    if(currHeight < 1 || currWidth < 1)
                        break;

                    std::string newPath;
                    if(!VirtualFileSystem::ResolvePhysicalPath(irrFiles[i], newPath))
                    {
                        LOG_FORMAT("Failed to load {0}", irrFiles[i].c_str());
                        failed = true;
                        break;
                    }

                    numMipsIrr++;
                }

                if(!failed)
                {
                    TextureDesc params;
                    params.srgb = true;
                    TextureLoadOptions loadOptions;
                    m_Environmnet   = SharedPtr<TextureCube>(GET_RHI_FACTORY()->CreateTextureCubeFromVCross(envFiles, m_NumMips, params, loadOptions));
                    m_IrradianceMap = SharedPtr<TextureCube>(GET_RHI_FACTORY()->CreateTextureCubeFromVCross(irrFiles, numMipsIrr, params, loadOptions));
                }
                else
                {
                    LOG("Failed to load environment");
                }

                delete[] envFiles;
                delete[] irrFiles;
            }
        }
        else // if (m_Mode == 1)
        {
            m_Parameters.w = m_Mode;
            GET_SCENE_RENDERER()->CreateCubeMap("", m_Parameters, m_Environmnet, m_IrradianceMap);
        }
    }

    Environment::~Environment()
    {
    }

    void Environment::SetEnvironmnet(TextureCube* environmnet)
    {
        m_Environmnet = SharedPtr<TextureCube>(environmnet);
    }

    void Environment::SetPrefilteredEnvironment(TextureCube* prefilteredEnvironment)
    {
        m_PrefilteredEnvironment = SharedPtr<TextureCube>(prefilteredEnvironment);
    }

    void Environment::SetIrradianceMap(TextureCube* irradianceMap)
    {
        m_IrradianceMap = SharedPtr<TextureCube>(irradianceMap);
    }
} // NekoEngine