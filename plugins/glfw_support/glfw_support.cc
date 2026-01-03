#include "glfw_support.hpp"

#include <GLFW/glfw3.h>

#include <print>

namespace glfw_support {

// 辅助函数：从窗口获取GLFWSupport实例
static inline GLFWSupport* get_instance(GLFWwindow* window) {
    return static_cast<GLFWSupport*>(glfwGetWindowUserPointer(window));
}

GLFWSupport::GLFWSupport(ecs::World& world)
    : window_(nullptr),
      world_(world),
      event_bus_(nullptr),
      width_(800),
      height_(600),
      title_(".") {}

GLFWSupport::~GLFWSupport() { cleanup(); }

// 插件生命周期管理
GLFWSupport* GLFWSupport::install(ecs::World& world) {
    // 确保EventBus存在
    if (!world.resource_manager().has<ecs::EventBus>()) {
        world.add_resource<ecs::EventBus>();
    }

    auto plugin = new GLFWSupport(world);
    plugin->event_bus_ = &world.resource<ecs::EventBus>();
    plugin->initialize();
    return plugin;
}

void GLFWSupport::uninstall(ecs::World& world, GLFWSupport* plugin) {
    delete plugin;
}

// 窗口管理接口实现
void GLFWSupport::create_window(int width, int height, const char* title) {
    width_ = width;
    height_ = height;
    title_ = title;
    init_glfw_window();
}

void GLFWSupport::destroy_window() { cleanup(); }

bool GLFWSupport::window_should_close() const {
    return window_ ? glfwWindowShouldClose(window_) : false;
}

void GLFWSupport::set_window_should_close(bool value) {
    if (window_) {
        glfwSetWindowShouldClose(window_, value ? GLFW_TRUE : GLFW_FALSE);
    }
}

void GLFWSupport::swap_buffers() const {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

void GLFWSupport::poll_events(double timeout_s) const {
    if (timeout_s > 0) glfwWaitEventsTimeout(timeout_s);
    glfwPollEvents();
}

// 输入处理接口实现
bool GLFWSupport::is_key_pressed(int key) const {
    return window_ ? glfwGetKey(window_, key) == GLFW_PRESS : false;
}

bool GLFWSupport::is_mouse_button_pressed(int button) const {
    return window_ ? glfwGetMouseButton(window_, button) == GLFW_PRESS : false;
}

void GLFWSupport::get_mouse_position(double& xpos, double& ypos) const {
    if (window_) {
        glfwGetCursorPos(window_, &xpos, &ypos);
    } else {
        xpos = 0.0;
        ypos = 0.0;
    }
}

// 初始化和清理方法
bool GLFWSupport::initialize() {
    std::println("GLFWSupport::initialize");
    if (!init_glfw()) {
        return false;
    }

    if (!init_glfw_window()) {
        return false;
    }

    return true;
}

bool GLFWSupport::init_glfw() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::println("Failed to initialize GLFW");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // 设置GLFW版本提示
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    std::println("GLFWSupport::init_glfw_window");
    return true;
}

bool GLFWSupport::init_glfw_window() {
    // 创建GLFW窗口
    window_ = glfwCreateWindow(width_, height_, title_, nullptr, nullptr);
    if (window_ == nullptr) {
        std::println("Failed to create GLFW window");
        throw std::runtime_error("Failed to create GLFW window");
    }

    // 设置窗口用户指针，用于回调函数
    glfwSetWindowUserPointer(window_, this);

    // Make the OpenGL context current
    glfwMakeContextCurrent(window_);

    // 设置GLFW回调函数
    glfwSetWindowCloseCallback(window_, window_close_callback);
    glfwSetWindowSizeCallback(window_, window_size_callback);
    glfwSetKeyCallback(window_, key_callback);
    glfwSetMouseButtonCallback(window_, mouse_button_callback);
    glfwSetCursorPosCallback(window_, cursor_pos_callback);

    // 设置交换间隔（垂直同步）
    glfwSwapInterval(1);
    std::println("GLFWSupport::init_glfw_window");
    return true;
}

void GLFWSupport::cleanup() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

// GLFW回调函数实现
void GLFWSupport::window_close_callback(GLFWwindow* window) {
    auto instance = get_instance(window);
    instance->event_bus_->publish<WindowCloseEvent>();
}

void GLFWSupport::window_size_callback(GLFWwindow* window, int width,
                                       int height) {
    auto instance = get_instance(window);
    instance->width_ = width;
    instance->height_ = height;
    instance->event_bus_->publish<WindowResizeEvent>(width, height);
}

void GLFWSupport::key_callback(GLFWwindow* window, int key, int scancode,
                               int action, int mods) {
    auto instance = get_instance(window);
    instance->event_bus_->publish<KeyEvent>(key, action, mods);
}

void GLFWSupport::mouse_button_callback(GLFWwindow* window, int button,
                                        int action, int mods) {
    auto instance = get_instance(window);
    double xpos, ypos;
    instance->get_mouse_position(xpos, ypos);
    instance->event_bus_->publish<MouseButtonEvent>(button, action, mods, xpos,
                                                    ypos);
}

void GLFWSupport::cursor_pos_callback(GLFWwindow* window, double xpos,
                                      double ypos) {
    auto instance = get_instance(window);
    instance->event_bus_->publish<MouseMoveEvent>(xpos, ypos);
}

const GetProcAddressFunc GLFWSupport::GetProcAddress = ::glfwGetProcAddress;
GLFWSupport& GLFWSupport::set_window_size(int width, int height) {
    if (window_) {
        glfwSetWindowSize(window_, width, height);
    }
    width_ = width;
    height_ = height;
    return *this;
}
GLFWSupport& GLFWSupport::set_window_title(const char* title) {
    if (window_) {
        glfwSetWindowTitle(window_, title);
    }
    title_ = title;
    return *this;
}
GLFWSupport& GLFWSupport::set_window_position(int xpos, int ypos) {
    if (window_) {
        glfwSetWindowPos(window_, xpos, ypos);
    }
    return *this;
}
}  // namespace glfw_support
