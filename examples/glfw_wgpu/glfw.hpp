#include <GLFW/glfw3.h>

#include <functional>

#include "logger.hpp"

class GlfwInstence {
   public:
    GlfwInstence() {
        if (instence_count.fetch_add(1, std::memory_order_acq_rel) == 0) {
            if (!glfwInit()) {
                XC_TLOG(ERROR, "glfwInit failed");
                throw std::runtime_error("glfwInit failed");
            } else {
                XC_TLOG(INFO, "glfwInit success");
            }
        }
        is_init_ = true;
    }
    ~GlfwInstence() {
        if (!is_init_) return;
        if (instence_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            glfwTerminate();
            XC_TLOG(INFO, "glfwTerminate success");
        }
    }
    GlfwInstence(const GlfwInstence&) = delete;
    GlfwInstence(GlfwInstence&&) = delete;
    GlfwInstence& operator=(const GlfwInstence&) = delete;
    GlfwInstence& operator=(GlfwInstence&&) = delete;

   private:
    bool is_init_{false};
    static std::atomic_size_t instence_count;
};

class GlfwWindow {
   public:
    GlfwWindow(int width, int height, std::string_view name)
        : width_(width), height_(height) {
        window_ =
            glfwCreateWindow(width, height, name.data(), nullptr, nullptr);
        if (!window_) {
            XC_TLOG(ERROR, "glfwCreateWindow failed");
            throw std::runtime_error("glfwCreateWindow failed");
        }
        XC_TLOG(INFO, "create window {} success", XC_SELF_PTR);
    }
    ~GlfwWindow() {
        if (window_) {
            glfwDestroyWindow(window_);
            XC_TLOG(INFO, "destroy window {} success", XC_SELF_PTR);
            window_ = nullptr;
        }
    }

    operator bool() const noexcept { return window_; }
    inline GLFWwindow* raw_window() const noexcept { return window_; }
    inline void make_current() const { glfwMakeContextCurrent(raw_window()); }
    inline bool should_close() const {
        return glfwWindowShouldClose(raw_window());
    }
    inline void swap_buffer() const { glfwSwapBuffers(raw_window()); }

    int width() const { return width_; }
    int height() const { return height_; }
    void set_framebuffer_size_callback(std::function<void(int, int)> callback) {
        framebuffer_size_callback_ = callback;
    }

   public:
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow(GlfwWindow&&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GlfwWindow& operator=(GlfwWindow&&) = delete;

   protected:
    void init_callback() {
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(
            window_, [](GLFWwindow* window, int width, int height) {
                auto s = self(window);
                s->width_ = width;
                s->height_ = height;
                if (s->framebuffer_size_callback_)
                    s->framebuffer_size_callback_(width, height);
            });
    }
    static inline GlfwWindow* self(GLFWwindow* window) {
        return (GlfwWindow*)glfwGetWindowUserPointer(window);
    }

   private:
    GLFWwindow* window_{nullptr};
    std::function<void(int, int)> framebuffer_size_callback_{nullptr};
    int width_{0};
    int height_{0};
};
inline std::atomic_size_t GlfwInstence::instence_count{0};
