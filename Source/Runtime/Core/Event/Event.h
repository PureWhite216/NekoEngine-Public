#pragma once
#include "Core.h"

#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { \
    return this->fn(std::forward<decltype(args)>(args)...);          \
}

#define EVENT_CLASS_TYPE(type)                      \
    static EventType GetStaticType()                \
    {                                               \
        return EventType::type;                     \
    }                                               \
    virtual EventType GetEventType() const override \
    {                                               \
        return GetStaticType();                     \
    }                                               \
    virtual const char* GetName() const override    \
    {                                               \
        return #type;                               \
    }

#define EVENT_CLASS_CATEGORY(category)            \
    virtual int GetCategoryFlags() const override \
    {                                             \
        return category;                          \
    }


namespace NekoEngine
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        WindowFile,
        AppTick,
        AppUpdate,
        AppRender,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        MouseEntered
    };

    enum EventCategory
    {
        None                     = 0,
        EventCategoryApplication = BIT(0),
        EventCategoryInput       = BIT(1),
        EventCategoryKeyboard    = BIT(2),
        EventCategoryMouse       = BIT(3),
        EventCategoryMouseButton = BIT(4)
    };

    class Event
    {
    protected:
        bool m_Handled = false;
    public:
        friend class EventDispatcher;

        virtual ~Event()                       = default;
        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const    = 0;
        virtual int GetCategoryFlags() const   = 0;
        virtual std::string ToString() const { return GetName(); }

        inline bool IsInCategory(EventCategory category)
        {
            return GetCategoryFlags() & category;
        }

        inline bool Handled() const { return m_Handled; }
    };

    class EventDispatcher
    {
    private:
        Event& m_Event;
    public:
        EventDispatcher(Event& event)
                : m_Event(event)
        {
        }

        template <typename T, typename F>
        bool Dispatch(const F& func)
        {
            if(m_Event.GetEventType() == T::GetStaticType())
            {
                m_Event.m_Handled = func(static_cast<T&>(m_Event));
                return true;
            }
            return false;
        }
    };
}