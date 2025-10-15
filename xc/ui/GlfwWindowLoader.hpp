#pragma once
#include <event/event.hpp>
#include "./AbsWindow.hpp"
struct GLFWwindow;
using ProcAddress = void (*(*)(const char*))(void);
class GlfwWindowLoader : EventPublisher {
   private:
    GLFWwindow* window_{nullptr};
    static size_t alive_window_count_;

   private:
    void init_();
    void init_callbacks_();

   public:
    GlfwWindowLoader(EventLoop* loop,const char* title="LearnOpenGL",int width=800,int height=600);
    GlfwWindowLoader();
    ~GlfwWindowLoader();
    void poll_events_timeout(double seconds);
    void make_current();
    void swap_buffers();
    ProcAddress get_proc_address();
    GLFWwindow* glfw_window_raw_ptr() const { return window_; }
    std::string_view window_title() const;

    void set_cursor_mode(CursorMode mode);
};
