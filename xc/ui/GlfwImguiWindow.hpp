#pragma once
#include <chrono>

#include "./AbsWindow.hpp"
#include "./Editor.hpp"
#include "./GlfwWindowLoader.hpp"
#include "./event.h"

class GlfwImguiWindow : public AbsWindow {
   public:
    using editor_list = std::vector<std::pair<std::unique_ptr<Editor>, bool>>;
    using button_list = std::vector<std::unique_ptr<Button>>;
    using element_list = std::variant<editor_list, button_list>;

   private:
    std::unique_ptr<GlfwWindowLoader> loader_{nullptr};
    std::unique_ptr<UiTimer> frame_timer_{nullptr};
    float fps_ = 60.f;
    ;
    std::vector<element_list> ui_elements_;

   private:
    inline void update_immediately_() { render_frame_(); }
    void render_frame_();

   protected:
    bool event(AbsEvent* event) override;

   protected:
    virtual void render();

   public:
    GlfwImguiWindow(const GlfwImguiWindow&) = delete;
    GlfwImguiWindow(GlfwImguiWindow&&) = delete;
    GlfwImguiWindow& operator=(const GlfwImguiWindow&) = delete;
    GlfwImguiWindow& operator=(GlfwImguiWindow&&) = delete;

   public:
    GlfwImguiWindow(const std::string& title = "GlfwImguiWindow",
                    int width = 800, int height = 600, int fps = 60);
    ~GlfwImguiWindow();
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
    Editor* add_editor(Fn fn, std::string_view title, bool sameline,
                       NamedArgs&&... args) {
        return append_element(
                   std::make_pair(std::make_unique<Editor>(
                                      std::forward<Fn>(fn), title,
                                      std::forward<NamedArgs>(args)...),
                                  sameline))
            .first.get();
    }
    template <typename Fn>
    Button* add_button(Fn&& fn, std::string_view label) {
        return append_element(
                   std::make_unique<Button>(label, std::forward<Fn>(fn)))
            .get();
    }

    template <typename T>
    T& append_element(T&& element);
};
