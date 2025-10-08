#include "./GlfwWindowLoader.hpp"
#include "./event.h"
#include <GLFW/glfw3.h>
#include <xc_assert.hpp>
void GlfwWindowLoader::init_callbacks_() {
#define M_self(w) \
    (static_cast<GlfwWindowLoader*>(glfwGetWindowUserPointer(window)))
    glfwSetScrollCallback(
        window_, [](GLFWwindow* window, double xoffset, double yoffset) {
            auto p = M_self(window_);
        });
    glfwSetCursorPosCallback(
        window_, [](GLFWwindow* window, double xpos, double ypos) {
            auto p = M_self(window);
            p->publish(std::make_unique<MouseMoveEvent>(xpos, ypos, p));
        });
    glfwSetMouseButtonCallback(
        window_, [](GLFWwindow* window, int button, int action, int mods) {
            auto p = M_self(window);
            MouseButton button_flag;
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                button_flag = MouseButtons::Left;
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                button_flag = MouseButtons::Right;
            } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                button_flag = MouseButtons::Middle;
            } else {
                return;
            }
            KeyAction action_flag;
            if (action == GLFW_PRESS) {
                action_flag = KeyActions::Press;
            } else if (action == GLFW_RELEASE) {
                action_flag = KeyActions::Release;
            } else if (action == GLFW_REPEAT) {
                action_flag = KeyActions::Repeat;
            } else {
                return;
            }
            Modifier modifiers_flag;
            if (mods & GLFW_MOD_SHIFT) {
                modifiers_flag |= Modifiers::Shift;
            }
            if (mods & GLFW_MOD_CONTROL) {
                modifiers_flag |= Modifiers::Control;
            }
            if (mods & GLFW_MOD_ALT) {
                modifiers_flag |= Modifiers::Alt;
            }
            if (mods & GLFW_MOD_SUPER) {
                modifiers_flag |= Modifiers::Super;
            }
            if (mods & GLFW_MOD_CAPS_LOCK) {
                modifiers_flag |= Modifiers::CapsLock;
            }
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            p->publish(std::make_unique<MouseButtonEvent>(
                button_flag, action_flag, modifiers_flag, xpos, ypos, p));
        });
}
void GlfwWindowLoader::poll_events_timeout(double seconds) {
    if (glfwWindowShouldClose(window_))
        publish(std::make_unique<Event>(EventType::WindowCloseRequested, this));
    glfwWaitEventsTimeout(seconds);
    loop()->flush();
}
void GlfwWindowLoader::swap_buffers() { glfwSwapBuffers(window_); }
void GlfwWindowLoader::make_current() { glfwMakeContextCurrent(window_); }

ProcAddress GlfwWindowLoader::get_proc_address() { return glfwGetProcAddress; }
size_t GlfwWindowLoader::alive_window_count_ = 0;
GlfwWindowLoader::GlfwWindowLoader(EventLoop* loop) : EventPublisher(loop) {
    init_();
}
GlfwWindowLoader::GlfwWindowLoader() : EventPublisher() { init_(); }
void GlfwWindowLoader::init_() {
    if (!alive_window_count_++) glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window_ == NULL) {
        std::cout << "Failed to create GLFW window_" << std::endl;
        --alive_window_count_;
        return;
    }
    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    init_callbacks_();
};
GlfwWindowLoader::~GlfwWindowLoader() {
    if (window_) glfwDestroyWindow(window_);
    if (!--alive_window_count_) glfwTerminate();
}
