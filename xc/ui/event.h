#pragma once
#include <event/event.hpp>
#include <event/timer.hpp>
#include <flags.hpp>

#include "./KeyCode.hpp"

using namespace std;

enum class EventType {
    Update,
    TimeOut,
    Render,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    Key,
    Wheel,
    WindowCloseRequested,
    WindowResized,
};

class Event : public AbsEvent {
    EventType type_id;
    void* sender_;

   public:
    Event(EventType type_id, void* sender)
        : type_id(type_id), sender_(sender) {}
    ~Event() override {}

    EventType type() const { return type_id; }
    void* sender() const { return sender_; }
};
class MouseMoveEvent : public Event {
    int x, y;

   public:
    MouseMoveEvent(int x, int y, void* sender)
        : Event(EventType::MouseMoved, sender), x(x), y(y) {}
    ~MouseMoveEvent() override {}

    int x_pos() const { return x; }
    int y_pos() const { return y; }
};
enum class MouseButtons : uint8_t {
    Left = 1 << 0,
    Right = 1 << 1,
    Middle = 1 << 2
};
enum class KeyActions : uint8_t { Press = 1, Release, Repeat };
enum class Modifiers : uint16_t {
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5,
};

using MouseButton = flags::Flags<MouseButtons>;
using KeyAction = flags::Flags<KeyActions>;
using Modifier = flags::Flags<Modifiers>;

class MouseButtonEvent : public Event {
    friend class GLFWEventPublisher;
    MouseButton button_;
    KeyAction action_;
    Modifier modifiers_;
    int x_, y_;

   public:
    MouseButtonEvent(MouseButton button, KeyAction action, Modifier modifiers,
                     int x, int y, void* sender)
        : Event(EventType::MouseButtonPressed, sender),
          button_(button),
          action_(action),
          modifiers_(modifiers),
          x_(x),
          y_(y) {}
    ~MouseButtonEvent() override {}

   public:
    MouseButton button() const { return button_; }
    KeyAction action() const { return action_; }
    Modifier modifiers() const { return modifiers_; }
    int x_pos() const { return x_; }
    int y_pos() const { return y_; }
};

using Cfg =
    EventConfig<Event, EventType, EventType::TimeOut, EventType::Update>;
using UiTimer = Timer<Cfg, [](void* p) -> std::unique_ptr<AbsEvent> {
    return std::make_unique<Event>(EventType::TimeOut, p);
}>;

class WindowResizeEvent : public Event {
   private:
    int width_, height_;

   public:
    WindowResizeEvent(int width, int height, void* sender)
        : Event(EventType::WindowResized, sender),
          width_(width),
          height_(height) {}
    ~WindowResizeEvent() override {}

    int width() const { return width_; }
    int height() const { return height_; }
};

class KeyEvent : public Event {
    Key key_;
    KeyAction action_;
    Modifier modifiers_;

   public:
    KeyEvent(Key key, KeyAction action, Modifier modifiers, void* sender)
        : Event(EventType::Key, sender),
          key_(key),
          action_(action),
          modifiers_(modifiers) {}
    ~KeyEvent() override {}

    Key key() const { return key_; }
    KeyAction action() const { return action_; }
    Modifier modifiers() const { return modifiers_; }
};

class WheelEvent : public Event {
    double x_offset_, y_offset_;

   public:
    WheelEvent(double x_offset, double y_offset, void* sender)
        : Event(EventType::Wheel, sender),
          x_offset_(x_offset),
          y_offset_(y_offset) {}
    ~WheelEvent() override {}

    double x_offset() const { return x_offset_; }
    double y_offset() const { return y_offset_; }
};
