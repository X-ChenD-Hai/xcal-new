#include "glfw_support.hpp"

#include <GLFW/glfw3.h>

#include <print>

#include "ecs/event_bus.hpp"

namespace glfw_support {

static Key glfw_to_key(int glfwKeycode) {
    // GLFW_KEYl-+*_C
    static const std::array<Key, 349> keyMap = {{
        /* Printable keys */
        Key::SPACE,          // 32
        Key::APOSTROPHE,     // 39
        Key::COMMA,          // 44
        Key::MINUS,          // 45
        Key::PERIOD,         // 46
        Key::SLASH,          // 47
        Key::_0,             // 48
        Key::_1,             // 49
        Key::_2,             // 50
        Key::_3,             // 51
        Key::_4,             // 52
        Key::_5,             // 53
        Key::_6,             // 54
        Key::_7,             // 55
        Key::_8,             // 56
        Key::_9,             // 57
        Key::SEMICOLON,      // 59
        Key::EQUAL,          // 61
        Key::A,              // 65
        Key::B,              // 66
        Key::C,              // 67
        Key::D,              // 68
        Key::E,              // 69
        Key::F,              // 70
        Key::G,              // 71
        Key::H,              // 72
        Key::I,              // 73
        Key::J,              // 74
        Key::K,              // 75
        Key::L,              // 76
        Key::M,              // 77
        Key::N,              // 78
        Key::O,              // 79
        Key::P,              // 80
        Key::Q,              // 81
        Key::R,              // 82
        Key::S,              // 83
        Key::T,              // 84
        Key::U,              // 85
        Key::V,              // 86
        Key::W,              // 87
        Key::X,              // 88
        Key::Y,              // 89
        Key::Z,              // 90
        Key::LEFT_BRACKET,   // 91
        Key::BACKSLASH,      // 92
        Key::RIGHT_BRACKET,  // 93
        Key::GRAVE_ACCENT,   // 96
        Key::WORLD_1,        // 161
        Key::WORLD_2,        // 162

        /* Function keys */
        Key::ESCAPE,         // 256
        Key::ENTER,          // 257
        Key::TAB,            // 258
        Key::BACKSPACE,      // 259
        Key::INSERT,         // 260
        Key::DELETE,         // 261
        Key::RIGHT,          // 262
        Key::LEFT,           // 263
        Key::DOWN,           // 264
        Key::UP,             // 265
        Key::PAGE_UP,        // 266
        Key::PAGE_DOWN,      // 267
        Key::HOME,           // 268
        Key::END,            // 269
        Key::CAPS_LOCK,      // 280
        Key::SCROLL_LOCK,    // 281
        Key::NUM_LOCK,       // 282
        Key::PRINT_SCREEN,   // 283
        Key::PAUSE,          // 284
        Key::F1,             // 290
        Key::F2,             // 291
        Key::F3,             // 292
        Key::F4,             // 293
        Key::F5,             // 294
        Key::F6,             // 295
        Key::F7,             // 296
        Key::F8,             // 297
        Key::F9,             // 298
        Key::F10,            // 299
        Key::F11,            // 300
        Key::F12,            // 301
        Key::F13,            // 302
        Key::F14,            // 303
        Key::F15,            // 304
        Key::F16,            // 305
        Key::F17,            // 306
        Key::F18,            // 307
        Key::F19,            // 308
        Key::F20,            // 309
        Key::F21,            // 310
        Key::F22,            // 311
        Key::F23,            // 312
        Key::F24,            // 313
        Key::F25,            // 314
        Key::KP_0,           // 320
        Key::KP_1,           // 321
        Key::KP_2,           // 322
        Key::KP_3,           // 323
        Key::KP_4,           // 324
        Key::KP_5,           // 325
        Key::KP_6,           // 326
        Key::KP_7,           // 327
        Key::KP_8,           // 328
        Key::KP_9,           // 329
        Key::KP_DECIMAL,     // 330
        Key::KP_DIVIDE,      // 331
        Key::KP_MULTIPLY,    // 332
        Key::KP_SUBTRACT,    // 333
        Key::KP_ADD,         // 334
        Key::KP_ENTER,       // 335
        Key::KP_EQUAL,       // 336
        Key::LEFT_SHIFT,     // 340
        Key::LEFT_CONTROL,   // 341
        Key::LEFT_ALT,       // 342
        Key::LEFT_SUPER,     // 343
        Key::RIGHT_SHIFT,    // 344
        Key::RIGHT_CONTROL,  // 345
        Key::RIGHT_ALT,      // 346
        Key::RIGHT_SUPER,    // 347
        Key::MENU            // 348
    }};

    if (glfwKeycode >= 0 && glfwKeycode <= static_cast<int>(Key::LAST)) {
        return static_cast<Key>(glfwKeycode);
    }
    return static_cast<Key>(-1);
}
static KeyAction glfw_to_action(int glfwAction) {
    if (glfwAction == GLFW_PRESS) {
        return KeyActions::Press;
    } else if (glfwAction == GLFW_RELEASE) {
        return KeyActions::Release;
    } else if (glfwAction == GLFW_REPEAT) {
        return KeyActions::Repeat;
    } else {
        return static_cast<KeyActions>(-1);
    }
}
static Modifier glfw_to_modifier(int glfwMods) {
    Modifier modifiers_flag;
    if (glfwMods & GLFW_MOD_SHIFT) {
        modifiers_flag |= Modifiers::Shift;
    }
    if (glfwMods & GLFW_MOD_CONTROL) {
        modifiers_flag |= Modifiers::Control;
    }
    if (glfwMods & GLFW_MOD_ALT) {
        modifiers_flag |= Modifiers::Alt;
    }
    if (glfwMods & GLFW_MOD_SUPER) {
        modifiers_flag |= Modifiers::Super;
    }
    if (glfwMods & GLFW_MOD_CAPS_LOCK) {
        modifiers_flag |= Modifiers::CapsLock;
    }
    if (glfwMods & GLFW_MOD_NUM_LOCK) {
        modifiers_flag |= Modifiers::NumLock;
    }
    return modifiers_flag;
}
static MouseButton glfw_to_button(int glfwButton) {
    using namespace glfw_support;
    std::println("parse");
    if (glfwButton == GLFW_MOUSE_BUTTON_LEFT) {
        return MouseButton::Left;
    } else if (glfwButton == GLFW_MOUSE_BUTTON_RIGHT) {
        return MouseButton::Right;
    } else if (glfwButton == GLFW_MOUSE_BUTTON_MIDDLE) {
        return MouseButton::Middle;
    }
    return static_cast<MouseButton>(0);
}

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

void GLFWSupport::poll_events(double timeout_s) {
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
    glfwSetScrollCallback(window_, wheel_callback);
    glfwSetCursorPosCallback(window_, mouse_move_callback);

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
    instance->event_bus_->publish<KeyEvent>(
        glfw_to_key(key), glfw_to_action(action), glfw_to_modifier(mods));
}

void GLFWSupport::mouse_button_callback(GLFWwindow* window, int button,
                                        int action, int mods) {
    auto instance = get_instance(window);
    double xpos, ypos;
    instance->get_mouse_position(xpos, ypos);
    instance->event_bus_->publish<MouseButtonEvent>(
        glfw_to_button(button), glfw_to_action(action), glfw_to_modifier(mods),
        xpos, ypos);
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
void GLFWSupport::wheel_callback(GLFWwindow* window, double xoffset,
                                 double yoffset) {
    auto instance = get_instance(window);
    instance->event_bus_->publish<WheelEvent>(xoffset, yoffset);
}
void GLFWSupport::mouse_move_callback(GLFWwindow* window, double xpos,
                                      double ypos) {
    auto instance = get_instance(window);
    instance->event_bus_->publish<MouseMoveEvent>(xpos - instance->last_pos_x_,
                                                  ypos - instance->last_pos_y_);
    instance->last_pos_x_ = xpos;
    instance->last_pos_y_ = ypos;
}

static inline int enum_to_glfw(InputMode mode) {
    switch (mode) {
        case InputMode::CursorNormal:
            return GLFW_CURSOR_NORMAL;
        case InputMode::CursorDisabled:
            return GLFW_CURSOR_DISABLED;
        default:
            return GLFW_CURSOR_NORMAL;
    }
}

void GLFWSupport::set_input_mode(InputMode mode) {
    if (window_) {
        glfwSetInputMode(window_, GLFW_CURSOR, enum_to_glfw(mode));
    }
}
void GLFWSupport::handle_extern_event() {
    event_bus_->each([&](SetInputModeEvent e) { set_input_mode(e.mode); });
}
}  // namespace glfw_support