#pragma once

#include "Event.h"
#include "KeyCode.h"
#include <sstream>

namespace NekoEngine
{
    class KeyEvent : public Event
    {
    protected:
        KeyCode m_KeyCode;

        KeyEvent(KeyCode keycode) : m_KeyCode(keycode)
        {
        }

    public:
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

        inline KeyCode GetKeyCode() const
        { return m_KeyCode; }
    };

    class KeyPressedEvent : public KeyEvent
    {
    private:
        int m_RepeatCount;

    public:
        KeyPressedEvent(KeyCode keycode, int repeatCount)
                : KeyEvent(keycode), m_RepeatCount(repeatCount)
        {
        }

        inline int GetRepeatCount() const
        { return m_RepeatCount; }

        std::string ToString() const override
        {
            std::stringstream ss{};
            ss << "KeyPressedEvent: " << uint32_t(m_KeyCode) << " (" << m_RepeatCount << " repeats)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode keycode)
                : KeyEvent(keycode)
        {
        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << uint32_t(m_KeyCode);
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };


    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(KeyCode keycode, char character)
                : KeyEvent(keycode), Character(character)
        {
        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << uint32_t(m_KeyCode);
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)

        char Character;

    private:
    };


}
