#pragma once
#include <chrono>
#include "./GlfwWindowLoader.hpp"
#include "./event.h"

using namespace std;



class Window : public EventPublisher, public EventListener {
   private:
    std::unique_ptr<GlfwWindowLoader> loader_{nullptr};
    bool stop_flag_ = false;
    std::unique_ptr<UiTimer> frame_timer_{nullptr};
    float fps_ = 60.f;

   private:
    inline void update_immediately_() { render_frame_(); }
    void render_frame_();

   protected:
   virtual bool mouse_move_event(MouseMoveEvent* e) {
        update_immediately_();
        return true;
    }
    virtual bool mouse_button_event(MouseButtonEvent* e) {
        update_immediately_();
        return true;
    }
    bool event(AbsEvent* event) override;

   public:
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

   public:
    Window();
    ~Window();
    inline void update() {
        publish(std::make_unique<Event>(EventType::Render, this));
    }
    inline void show() {
        stop_flag_ = false;
        frame_timer_ = std::make_unique<UiTimer>();
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            1000ms / fps_);
        ;
        frame_timer_->start(interval, true);
        publish(std::make_unique<Event>(EventType::Update, this));
        publish(std::make_unique<Event>(EventType::Render, this));
        while (!stop_flag_) {
            publish(std::make_unique<Event>(EventType::Update, this));
            loader_->poll_events_timeout(0.005);
            loop()->flush();
        }
    }
};