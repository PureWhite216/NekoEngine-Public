#pragma once
#include "Core.h"
#include "Serialization/ISerializable.h"
#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "entt/entt.hpp"
#include "LevelGraph.h"
#include "Timer/TimeStep.h"
#include "Renderable/EditorCamera.h"
#include "Event/ApplicationEvent.h"

namespace NekoEngine
{

    struct LevelRenderSettings
    {
        bool Renderer2DEnabled         = true;
        bool Renderer3DEnabled         = true;
        bool DebugRenderEnabled        = true;
        bool SkyboxRenderEnabled       = true;
        bool ShadowsEnabled            = true;
        bool FXAAEnabled               = true;
        bool DebandingEnabled          = true;
        bool ChromaticAberationEnabled = false;
        bool EyeAdaptation             = false;
        bool SSAOEnabled               = false;
        bool BloomEnabled              = true;
        bool FilmicGrainEnabled        = false;
        bool MotionBlurEnabled         = false;
        bool DepthOfFieldEnabled       = false;
        bool SharpenEnabled            = false;
        float DepthOfFieldStrength     = 1.0f;
        float DepthOfFieldDistance     = 100.0f;

        // Shadow Settings
        float m_CascadeSplitLambda = 0.92f;
        float m_LightSize          = 1.5f;
        float m_MaxShadowDistance  = 400.0f;
        float m_ShadowFade         = 40.0f;
        float m_CascadeFade        = 3.0f;
        float m_InitialBias        = 0.0023f;
        uint32_t m_ShadowMapNum    = 4;
        uint32_t m_ShadowMapSize   = 2048;

        float m_Exposure        = 1.0f;
        uint32_t m_ToneMapIndex = 6;
        float Brightness        = 0.0f;
        float Saturation        = 1.0f;
        float Contrast          = 1.0f;

        // Bloom
        float m_BloomIntensity   = 1.0f;
        float BloomThreshold     = 1.0f;
        float BloomKnee          = 0.1f;
        float BloomUpsampleScale = 1.0f;

        // SSAO
        int SSAOBlurRadius     = 4;
        float SSAOSampleRadius = 4.0f;
        bool SSAOBlur          = true;
        float SSAOStrength     = 1.0f;

        float SkyboxMipLevel = 0.0f;
        int DebugMode        = 0;
    };

    struct LevelPhysicsSettings
    {
        uint32_t m_MaxUpdatesPerFrame = 5;
        uint32_t VelocityIterations   = 20;
        uint32_t PositionIterations   = 1;

        glm::vec3 Gravity             = glm::vec3(0.0f, -9.81f, 0.0f);
        float Dampening               = 0.9995f;
        uint32_t IntegrationTypeIndex = 3;
        uint32_t BroadPhaseTypeIndex  = 2;
    };

    struct LevelSettings
    {
        bool PhysicsEnabled2D = false;
        bool PhysicsEnabled3D = true;
        bool AudioEnabled     = false;

        LevelRenderSettings renderSettings;
        LevelPhysicsSettings physicsSettings;
    };

    class EntityManager;
    class Entity;

    class Level
    {
    public:
         String levelName;
    protected:
         LevelSettings sceneSettings;
         UniquePtr<EntityManager> m_EntityManager;
         UniquePtr<LevelGraph> m_LevelGraph;
        uint32_t screenWidth;
        uint32_t screenHeight;
    private:
        friend class Entity;
    public:
        Level(const String& name);
        virtual ~Level();

        Level(const Level&) = delete;
        Level& operator=(const Level&) = delete;

        virtual void Init();

        virtual void OnUpdate(const TimeStep& timeStep);

        virtual void OnImGui(){};

        virtual void OnCleanup();;
        
        virtual void OnEvent(Event& e);

        virtual void Serialise(const std::string& filePath, bool binary = false);
        virtual void Deserialise(const std::string& filePath, bool binary = false);

        void DeleteAllGameObjects();

        virtual void Render3D()
        {}
        virtual void Render2D()
        {}

        void UpdateLevelGraph();

        void SetScreenSize(uint32_t width, uint32_t height);

        uint32_t GetScreenWidth() const
        {
            return screenWidth;
        }

        uint32_t GetScreenHeight() const
        {
            return screenHeight;
        }

        void DuplicateEntity(Entity entity);
        void DuplicateEntity(Entity entity, Entity parent);
        Entity CreateEntity();
        Entity CreateEntity(const std::string& name);
        Entity GetEntityByUUID(uint64_t id);
        Entity InstantiatePrefab(const std::string& path);
        void SavePrefab(Entity entity, const std::string& path);


        EntityManager* GetEntityManager() { return m_EntityManager.get(); }

        entt::registry& GetRegistry();

        const String& GetName() const { return levelName; }

        LevelSettings& GetSettings()
        {
            return sceneSettings;
        }

        void SetName(const std::string& _name)
        {
            levelName = _name;
        }

        void SetScreenWidth(uint32_t width)
        {
            screenWidth = width;
        }
        void SetScreenHeight(uint32_t height)
        {
            screenHeight = height;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Scene Name", levelName));

            archive(cereal::make_nvp("PhysicsEnabled2D", sceneSettings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", sceneSettings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", sceneSettings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", sceneSettings.renderSettings.Renderer2DEnabled),
                    cereal::make_nvp("Renderer3DEnabled", sceneSettings.renderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", sceneSettings.renderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", sceneSettings.renderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", sceneSettings.renderSettings.ShadowsEnabled));
            archive(cereal::make_nvp("Exposure", sceneSettings.renderSettings.m_Exposure), cereal::make_nvp("ToneMap", sceneSettings.renderSettings.m_ToneMapIndex));

            archive(cereal::make_nvp("BloomIntensity", sceneSettings.renderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", sceneSettings.renderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", sceneSettings.renderSettings.BloomThreshold),
                    cereal::make_nvp("BloomUpsampleScale", sceneSettings.renderSettings.BloomUpsampleScale));

            archive(cereal::make_nvp("FXAAEnabled", sceneSettings.renderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", sceneSettings.renderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", sceneSettings.renderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", sceneSettings.renderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", sceneSettings.renderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", sceneSettings.renderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", sceneSettings.renderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", sceneSettings.renderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", sceneSettings.renderSettings.MotionBlurEnabled));

            archive(cereal::make_nvp("DepthOFFieldEnabled", sceneSettings.renderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", sceneSettings.renderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", sceneSettings.renderSettings.DepthOfFieldDistance));

            archive(sceneSettings.renderSettings.Brightness, sceneSettings.renderSettings.Saturation, sceneSettings.renderSettings.Contrast);

            archive(sceneSettings.renderSettings.SharpenEnabled);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Scene Name", levelName));

            archive(cereal::make_nvp("PhysicsEnabled2D", sceneSettings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", sceneSettings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", sceneSettings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", sceneSettings.renderSettings.Renderer2DEnabled),
                    cereal::make_nvp("Renderer3DEnabled", sceneSettings.renderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", sceneSettings.renderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", sceneSettings.renderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", sceneSettings.renderSettings.ShadowsEnabled));
            archive(cereal::make_nvp("Exposure", sceneSettings.renderSettings.m_Exposure), cereal::make_nvp("ToneMap", sceneSettings.renderSettings.m_ToneMapIndex));

            archive(cereal::make_nvp("BloomIntensity", sceneSettings.renderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", sceneSettings.renderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", sceneSettings.renderSettings.BloomThreshold),
                    cereal::make_nvp("BloomUpsampleScale", sceneSettings.renderSettings.BloomUpsampleScale));
            archive(cereal::make_nvp("FXAAEnabled", sceneSettings.renderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", sceneSettings.renderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", sceneSettings.renderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", sceneSettings.renderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", sceneSettings.renderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", sceneSettings.renderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", sceneSettings.renderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", sceneSettings.renderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", sceneSettings.renderSettings.MotionBlurEnabled));

            archive(cereal::make_nvp("DepthOfFieldEnabled", sceneSettings.renderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", sceneSettings.renderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", sceneSettings.renderSettings.DepthOfFieldDistance));

            archive(sceneSettings.renderSettings.Brightness, sceneSettings.renderSettings.Saturation, sceneSettings.renderSettings.Contrast);

            archive(sceneSettings.renderSettings.SharpenEnabled);
        }
        
    protected:
        bool OnWindowResize(WindowResizeEvent& e);
    };



    class DefaultCameraController
    {
    public:
        enum class ControllerType : int
        {
            FPS = 0,
            ThirdPerson,
            Simple,
            Camera2D,
            EditorCamera,
            Custom
        };

        DefaultCameraController()
                : m_Type(ControllerType::Custom)
        {
        }

        DefaultCameraController(ControllerType type)
        {
            SetControllerType(type);
        }

        void SetControllerType(ControllerType type)
        {
            // if(type != m_Type)
            {
                m_Type = type;
                switch(type)
                {
//                    case ControllerType::ThirdPerson:
//                        m_CameraController = MakeShared<ThirdPersonCameraController>();
//                        break;
//                    case ControllerType::FPS:
//                        m_CameraController = MakeShared<FPSCameraController>();
//                        break;
//                    case ControllerType::Simple:
//                        m_CameraController = MakeShared<FPSCameraController>();
//                        break;
                    case ControllerType::EditorCamera:
                        m_CameraController = MakeShared<EditorCameraController>();
                        break;
                    case ControllerType::Custom:
                        m_CameraController = nullptr;
                        break;
                }
            }
        }

        static std::string CameraControllerTypeToString(ControllerType type)
        {
            switch(type)
            {
                case ControllerType::ThirdPerson:
                    return "ThirdPerson";
                case ControllerType::FPS:
                    return "FPS";
                case ControllerType::Simple:
                    return "Simple";
                case ControllerType::EditorCamera:
                    return "Editor";
                case ControllerType::Camera2D:
                    return "2D";
                case ControllerType::Custom:
                    return "Custom";
            }

            return "Custom";
        }

        static ControllerType StringToControllerType(const std::string& type)
        {
            if(type == "ThirdPerson")
                return ControllerType::ThirdPerson;
            if(type == "FPS")
                return ControllerType::FPS;
            if(type == "Simple")
                return ControllerType::Simple;
            if(type == "Editor")
                return ControllerType::EditorCamera;
            if(type == "2D")
                return ControllerType::Camera2D;
            if(type == "Custom")
                return ControllerType::Custom;

            LOG_FORMAT("Unsupported Camera controller %s", type.c_str());
            return ControllerType::Custom;
        }

        const SharedPtr<CameraController>& GetController() const
        {
            return m_CameraController;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("ControllerType", m_Type));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("ControllerType", m_Type));
            SetControllerType(m_Type);
        }

        ControllerType GetType()
        {
            return m_Type;
        }

    private:
        ControllerType m_Type = ControllerType::Custom;
        SharedPtr<CameraController> m_CameraController;
    };


} // NekoEngine

