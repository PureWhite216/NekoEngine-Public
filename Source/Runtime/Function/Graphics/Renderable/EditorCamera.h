#pragma once
#include "CameraController.h"

namespace NekoEngine
{

    enum class EditorCameraMode
    {
        NONE,
        FLYCAM,
        ARCBALL,
        TWODIM
    };

    class EditorCameraController : public CameraController
    {
    private:
        EditorCameraMode m_CameraMode = EditorCameraMode::ARCBALL;
        glm::vec2 m_StoredCursorPos;
        float m_CameraSpeed = 20.0f;

        float m_PitchDelta { 0.0f }, m_YawDelta { 0.0f };
        glm::vec3 m_PositionDelta {};
        
    public:
        EditorCameraController();
        ~EditorCameraController();

        virtual void HandleMouse(Transform& transform, float dt, float xpos, float ypos) override;
        virtual void HandleKeyboard(Transform& transform, float dt) override;

        void MousePan(Transform& transform, const glm::vec2& delta);
        void MouseRotate(Transform& transform, const glm::vec2& delta);
        void MouseZoom(Transform& transform, float delta);
        void UpdateCameraView(Transform& transform, float dt);

        glm::vec3 CalculatePosition(Transform& transform) const;
        std::pair<float, float> PanSpeed() const;
        float RotationSpeed() const;
        float ZoomSpeed() const;

        void UpdateScroll(Transform& transform, float offset, float dt) override;

        void StopMovement();
        void SetSpeed(float speed) { m_CameraSpeed = speed; }

        void SetCurrentMode(EditorCameraMode mode) { m_CameraMode = mode; }
        EditorCameraMode GetCurrentMode() const { return m_CameraMode; }
    };

} // NekoEngine

