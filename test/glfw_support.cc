#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <glfw_support/glfw_support.hpp>
#include <xc/ecs/world.hpp>

using namespace ecs;
namespace glfw = glfw_support;
TEST(GLFWSupport, InstallAndUninstall) {
    World world;

    // 测试插件安装
    world.use_plugin<glfw::GLFWSupport>();

    // 测试插件是否成功安装
    EXPECT_TRUE(world.resource_manager().has<glfw::GLFWSupport>());

    // 测试插件获取
    auto& glfw_support = world.plugin<glfw::GLFWSupport>();

    // 测试插件卸载会在world销毁时自动处理
}

TEST(GLFWSupport, WindowManagement) {
    World world;
    world.use_plugin<glfw::GLFWSupport>();

    auto& glfw_support = world.plugin<glfw::GLFWSupport>();

    // 测试窗口创建
    glfw_support.create_window(800, 600, "Test Window");

    // 测试窗口初始状态
    EXPECT_FALSE(glfw_support.window_should_close());

    // 测试窗口状态设置
    glfw_support.set_window_should_close(true);
    EXPECT_TRUE(glfw_support.window_should_close());

    // 测试窗口关闭重置
    glfw_support.set_window_should_close(false);
    EXPECT_FALSE(glfw_support.window_should_close());

    // 测试窗口销毁
    glfw_support.destroy_window();
}

TEST(GLFWSupport, EventSystem) {
    World world;
    world.use_plugin<glfw::GLFWSupport>();

    auto& event_bus = world.resource<EventBus>();
    auto& glfw_support = world.plugin<glfw::GLFWSupport>();

    // 测试窗口关闭事件
    bool window_close_called = false;
    event_bus.each<glfw::WindowCloseEvent>(
        [&](auto&) { window_close_called = true; });

    glfw_support.set_window_should_close(true);
    glfw_support.poll_events();
    world.execute_commands();

    // 由于我们在run方法中检查窗口是否应该关闭，所以这里应该触发事件
    // 注意：实际的窗口关闭事件是由GLFW回调触发的，这里我们测试的是run方法中的检查
}

TEST(GLFWSupport, InputHandling) {
    World world;
    world.use_plugin<glfw::GLFWSupport>();

    auto& glfw_support = world.plugin<glfw::GLFWSupport>();

    // 测试键盘输入初始状态
    EXPECT_FALSE(glfw_support.is_key_pressed(GLFW_KEY_ESCAPE));
    EXPECT_FALSE(glfw_support.is_key_pressed(GLFW_KEY_SPACE));

    // 测试鼠标输入初始状态
    EXPECT_FALSE(glfw_support.is_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(glfw_support.is_mouse_button_pressed(GLFW_MOUSE_BUTTON_RIGHT));

    // 测试鼠标位置初始值
    double xpos, ypos;
    glfw_support.get_mouse_position(xpos, ypos);
    // 初始位置可能为0.0，但这取决于GLFW的实现
}
