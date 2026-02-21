#include <GLFW/glfw3.h>
//
#include "./glfw_window_loader.hpp"


#include <xc/common/xc_assert.hpp>

#include "./event.h"
#include "./glfw_dark_header_support.h"
#include "./key_code.hpp"

static Key glfwToKey(int glfwKeycode) {
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
static KeyAction glfwToAction(int glfwAction) {
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
static Modifier glfwToModifier(int glfwMods) {
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

void GlfwWindowLoader::init_callbacks_() {
#define M_self(w) \
    (static_cast<GlfwWindowLoader*>(glfwGetWindowUserPointer(window)))
    glfwSetScrollCallback(
        window_, [](GLFWwindow* window, double xoffset, double yoffset) {
            auto p = M_self(window_);
            p->publish(std::make_unique<WheelEvent>(xoffset, yoffset, p));
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
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            p->publish(std::make_unique<MouseButtonEvent>(
                button_flag, glfwToAction(action), glfwToModifier(mods), xpos,
                ypos, p));
        });
    glfwSetWindowSizeCallback(
        window_, [](GLFWwindow* window, int width, int height) {
            auto p = M_self(window);
            p->publish(std::make_unique<WindowResizeEvent>(width, height, p));
        });
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode,
                                   int action, int mods) {
        auto p = M_self(window);
        p->publish(std::make_unique<KeyEvent>(
            glfwToKey(key), glfwToAction(action), glfwToModifier(mods), p));
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
GlfwWindowLoader::GlfwWindowLoader(EventLoop* loop, const char* title,
                                   int width, int height)
    : EventPublisher(loop) {
    init_();
    glfwSetWindowTitle(window_, title);
    glfwSetWindowSize(window_, width, height);
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
    enable_window_dark_titlebar(window_);
    init_callbacks_();
};
GlfwWindowLoader::~GlfwWindowLoader() {
    if (window_) glfwDestroyWindow(window_);
    if (!--alive_window_count_) glfwTerminate();
}
std::string_view GlfwWindowLoader::window_title() const {
    return glfwGetWindowTitle(window_);;
}
void GlfwWindowLoader::set_cursor_mode(CursorMode mode) {
    if (mode == CursorMode::Normal) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else if (mode == CursorMode::Hidden) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else if (mode == CursorMode::Locked) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else if (mode == CursorMode::Disabled) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}
