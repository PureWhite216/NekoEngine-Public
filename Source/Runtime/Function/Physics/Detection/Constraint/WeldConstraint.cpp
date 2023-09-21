#include "Renderer/DebugRenderer.h"
#include "WeldConstraint.h"

namespace NekoEngine
{
    WeldConstraint::WeldConstraint(RigidBody3D* obj1, RigidBody3D* obj2)
            : m_pObj1(obj1)
            , m_pObj2(obj2)
            , m_positionOffset(obj2->GetPosition() - obj1->GetPosition())
            , m_orientation(obj2->GetOrientation())
    {
    }

    void WeldConstraint::ApplyImpulse()
    {

        // Position
        glm::vec3 pos(m_positionOffset);
        pos = m_pObj1->GetOrientation() * pos;
        pos += m_pObj1->GetPosition();
        m_pObj2->SetPosition(pos);

        // Orientation
        m_pObj2->SetOrientation(m_pObj1->GetOrientation() * m_orientation);
    }

    void WeldConstraint::DebugDraw() const
    {
        glm::vec3 posA = m_pObj1->GetPosition();
        glm::vec3 posB = m_pObj2->GetPosition();

        DebugRenderer::DrawThickLine(posA, posB, 0.02f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        DebugRenderer::DrawPointNDT(posA, 0.05f, glm::vec4(1.0f, 0.8f, 1.0f, 1.0f));
        DebugRenderer::DrawPointNDT(posB, 0.05f, glm::vec4(1.0f, 0.8f, 1.0f, 1.0f));
    }
} // NekoEngine