#pragma once
#include "Core.h"
#include "Transform.h"
#include "Math/Maths.h"
#include "Camera.h"

namespace NekoEngine
{
    class CameraController
    {
    protected:
        glm::vec3 m_Velocity;
        glm::vec2 m_RotateVelocity;
        glm::vec2 m_ArcRotateVelocity;
        glm::vec3 m_FocalPoint;

        float m_ZoomVelocity = 0.0f;
        float m_CameraSpeed  = 0.0f;
        float m_Distance     = 0.0f;
        float m_Zoom         = 1.0f;

        glm::vec2 m_ProjectionOffset  = glm::vec2(0.0f, 0.0f);
        glm::vec2 m_PreviousCurserPos = glm::vec2(0.0f, 0.0f);
        float m_MouseSensitivity      = 0.1f;

        float m_ZoomDampeningFactor   = 0.00001f;
        float m_DampeningFactor       = 0.00001f;
        float m_RotateDampeningFactor = 0.001f;

        Camera* m_Camera = nullptr;

    public:
        CameraController()          = default;
        virtual ~CameraController() = default;

        virtual void HandleMouse(Transform& transform, float dt, float xpos, float ypos) {};
        virtual void HandleKeyboard(Transform& transform, float dt) {};
        virtual void UpdateScroll(Transform& transform, float offset, float dt) {};

        virtual void OnImGui() {};

        void SetMouseSensitivity(float value) { m_MouseSensitivity = value; }

        const glm::vec3& GetVelocity() const { return m_Velocity; }

        void SetCamera(Camera* camera) { m_Camera = camera; }
    };



} // NekoEngine

