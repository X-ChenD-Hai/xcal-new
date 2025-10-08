#pragma once
#include "./event.h"
class AbsWindow : public EventPublisher, public EventListener {
    bool stop_flag_ = false;

   public:
    AbsWindow() = default;
    void close() { stop_flag_ = true; }
    bool is_showing() { return !stop_flag_; }

   protected:
    virtual bool mouse_move_event(MouseMoveEvent*) { return true; }
    virtual bool mouse_button_event(MouseButtonEvent*) { return true; }

   protected:
    virtual bool ready_to_show() { return true; }
    virtual void update_frame() = 0;

   public:
    void show() {
        stop_flag_ = !ready_to_show();
        while (is_showing()) {
            publish(std::make_unique<Event>(EventType::Update, this));
            loop()->flush();
            update_frame();
        }
    }
};