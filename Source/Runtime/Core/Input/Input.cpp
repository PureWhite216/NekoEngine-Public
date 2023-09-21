
#include "Input.h"

namespace NekoEngine
{
    Input::Input()
            : m_MouseMode(MouseMode::Visible)
    {
        Reset();
    }

    void Input::Reset()
    {
        memset(m_KeyHeld, 0, MAX_KEYS);
        memset(m_KeyPressed, 0, MAX_KEYS);
        memset(m_MouseClicked, 0, MAX_BUTTONS);
        memset(m_MouseHeld, 0, MAX_BUTTONS);

        m_MouseOnScreen = true;
        m_ScrollOffset  = 0.0f;
    }

    void Input::ResetPressed()
    {
        memset(m_KeyPressed, 0, MAX_KEYS);
        memset(m_MouseClicked, 0, MAX_BUTTONS);
        m_ScrollOffset = 0;
    }

    void Input::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(Input::OnKeyPressed));
        dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(Input::OnKeyReleased));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(Input::OnMousePressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(Input::OnMouseReleased));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(Input::OnMouseScrolled));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(Input::OnMouseMoved));
        dispatcher.Dispatch<MouseEnterEvent>(BIND_EVENT_FN(Input::OnMouseEnter));
    }

    bool Input::OnKeyPressed(KeyPressedEvent& e)
    {
        SetKeyPressed(KeyCode(e.GetKeyCode()), e.GetRepeatCount() < 1);
        SetKeyHeld(KeyCode(e.GetKeyCode()), true);
        return false;
    }

    bool Input::OnKeyReleased(KeyReleasedEvent& e)
    {
        SetKeyPressed(KeyCode(e.GetKeyCode()), false);
        SetKeyHeld(KeyCode(e.GetKeyCode()), false);
        return false;
    }

    bool Input::OnMousePressed(MouseButtonPressedEvent& e)
    {
        SetMouseClicked(MouseKeyCode(e.GetMouseButton()), true);
        SetMouseHeld(MouseKeyCode(e.GetMouseButton()), true);
        return false;
    }

    bool Input::OnMouseReleased(MouseButtonReleasedEvent& e)
    {
        SetMouseClicked(MouseKeyCode(e.GetMouseButton()), false);
        SetMouseHeld(MouseKeyCode(e.GetMouseButton()), false);
        return false;
    }

    bool Input::OnMouseScrolled(MouseScrolledEvent& e)
    {
        SetScrollOffset(e.GetYOffset());
        return false;
    }

    bool Input::OnMouseMoved(MouseMovedEvent& e)
    {
        StoreMousePosition(e.GetX(), e.GetY());
        return false;
    }

    bool Input::OnMouseEnter(MouseEnterEvent& e)
    {
        SetMouseOnScreen(e.GetEntered());
        return false;
    }

    bool Input::IsControllerPresent(int id)
    {
        return m_Controllers.find(id) != m_Controllers.end();
    }

    std::vector<int> Input::GetConnectedControllerIDs()
    {
        std::vector<int> ids;
        ids.reserve(m_Controllers.size());
        for(auto [id, controller] : m_Controllers)
            ids.emplace_back(id);

        return ids;
    }

    Controller* Input::GetController(int id)
    {
        if(!Input::IsControllerPresent(id))
            return nullptr;

        return &m_Controllers.at(id);
    }

    Controller* Input::GetOrAddController(int id)
    {
        return &(m_Controllers[id]);
    }

    std::string Input::GetControllerName(int id)
    {
        if(!Input::IsControllerPresent(id))
            return {};

        return m_Controllers.at(id).Name;
    }

    bool Input::IsControllerButtonPressed(int controllerID, int button)
    {
        if(!Input::IsControllerPresent(controllerID))
            return false;

        const Controller& controller = m_Controllers.at(controllerID);
        if(controller.ButtonStates.find(button) == controller.ButtonStates.end())
            return false;

        return controller.ButtonStates.at(button);
    }

    float Input::GetControllerAxis(int controllerID, int axis)
    {
        if(!Input::IsControllerPresent(controllerID))
            return 0.0f;

        const Controller& controller = m_Controllers.at(controllerID);
        if(controller.AxisStates.find(axis) == controller.AxisStates.end())
            return 0.0f;

        return controller.AxisStates.at(axis);
    }

    uint8_t Input::GetControllerHat(int controllerID, int hat)
    {
        if(!Input::IsControllerPresent(controllerID))
            return 0;

        const Controller& controller = m_Controllers.at(controllerID);
        if(controller.HatStates.find(hat) == controller.HatStates.end())
            return 0;

        return controller.HatStates.at(hat);
    }

    void Input::RemoveController(int id)
    {
        for(auto it = m_Controllers.begin(); it != m_Controllers.end();)
        {
            int currentID = it->first;
            if(currentID == id)
            {
                m_Controllers.erase(it);
                return;
            }
            else
                it++;
        }
    }

} // NekoEngine