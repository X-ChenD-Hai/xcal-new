#pragma once
#include <chrono>

#include "./AbsWindow.hpp"
#include "./Editor.hpp"
#include "./GlfwWindowLoader.hpp"
#include "./event.h"

class Window final : public AbsWindow {
   private:
    std::unique_ptr<GlfwWindowLoader> loader_{nullptr};
    std::unique_ptr<UiTimer> frame_timer_{nullptr};
    float fps_ = 60.f;
    std::vector<std::unique_ptr<Editor>> editors_;

   private:
    inline void update_immediately_() { render_frame_(); }
    void render_frame_();

   protected:
    bool event(AbsEvent* event) override;

   protected:
    virtual void render();

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
    bool ready_to_show() override {
        frame_timer_ = std::make_unique<UiTimer>();
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            1000ms / fps_);
        ;
        frame_timer_->start(interval, true);
        publish(std::make_unique<Event>(EventType::Update, this));
        publish(std::make_unique<Event>(EventType::Render, this));

        return true;
    }
    void update_frame() override { loader_->poll_events_timeout(0.005); };
    template <typename Fn, typename... NamedArgs>
    Editor* add_editor(Fn fn, std::string title, NamedArgs&&... args) {
        return editors_
            .emplace_back(new Editor(Editor(std::forward<Fn>(fn), title,
                                            std::forward<NamedArgs>(args)...)))
            .get();
    }
};