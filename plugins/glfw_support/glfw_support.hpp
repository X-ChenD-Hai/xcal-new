#pragma once
#include <ecs/command/command.hpp>
#include <ecs/command_submit.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/world.hpp>

#include "ui_protocol/types.hpp"

struct GLFWwindow;
namespace glfw_support {
using namespace ui_protocol;
class GLFWSupport {
    friend class ::ecs::World;

   public:
    // 窗口管理接口
    void create_window(int width, int height, const char* title);
    GLFWSupport& set_window_size(int width, int height);
    GLFWSupport& set_window_title(const char* title);
    GLFWSupport& set_window_position(int xpos, int ypos);
    void destroy_window();
    bool window_should_close() const;
    void set_window_should_close(bool value);
    void set_input_mode(InputMode mode);
    void poll_events(double timeout_s = 0.0);
    void swap_buffers() const;
    void handle_extern_event() ;

    // 输入处理接口
    bool is_key_pressed(int key) const;
    bool is_mouse_button_pressed(int button) const;
    void get_mouse_position(double& xpos, double& ypos) const;

    static const GetProcAddressFunc GetProcAddress;

   protected:
    // 插件生命周期管理
    static GLFWSupport* install(ecs::World& world);
    static void uninstall(ecs::World& world, GLFWSupport* plugin);

   private:
    GLFWSupport(ecs::World& world);
    ~GLFWSupport();

    bool initialize();
    bool init_glfw();
    bool init_glfw_window();
    void cleanup();

    // GLFW回调函数
    static void window_close_callback(GLFWwindow* window);
    static void window_size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode,
                             int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button,
                                      int action, int mods);
    static void cursor_pos_callback(GLFWwindow* window, double xpos,
                                    double ypos);
    static void wheel_callback(GLFWwindow* window, double xoffset,
                               double yoffset);
    static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);

   private:
    GLFWwindow* window_;
    ecs::World& world_;
    ecs::EventBus* event_bus_;
    int width_;
    int height_;
    const char* title_;
    double last_pos_x_, last_pos_y_;
};

}  // namespace glfw_support
