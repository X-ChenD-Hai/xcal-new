#include <print>

#include "ecs/event_bus.hpp"
#include "ecs/world.hpp"
#include "glfw_support.hpp"
#include "opengl_support.hpp"
#include "render.hpp"
#include "ui_protocol/types.hpp"
namespace glfw = glfw_support;

static constexpr int kWindowWidth = 800;
static constexpr int kWindowHeight = 600;

void run(glfw::GLFWSupport& glfw, ecs::World& world, ecs::EventBus& event_bus) {
    while (!glfw.window_should_close()) {
        glfw.poll_events();
        event_bus.each([&](glfw::WindowResizeEvent& e) {
            std::println("WindowResizeEvent {} {}", e.width, e.height);
            auto ev = e;
            event_bus.publish<opengl_support::FrameResizeEvent>(ev.width,
                                                                ev.height);
        });
        world.run_system<&app::Renderer::run>();
        glfw.swap_buffers();
        glfw.handle_extern_event();
        world.execute_commands();
        event_bus.clear();
    }
    glfw.destroy_window();
}

int main() {
    using namespace ecs;
    World world;
    world.use_plugin<glfw::GLFWSupport>();
    std::println("glfw::GLFWSupport::glfwGetProcAddress {}",
                 (void*)glfw::GLFWSupport::GetProcAddress);
    world.use_plugin<opengl_support::OpenGLSupport>(
        glfw::GLFWSupport::GetProcAddress);
    world.resource<glfw::GLFWSupport>()
        .set_window_size(kWindowWidth, kWindowHeight)
        .set_window_title("GLFW OpenGL Example")
        .set_window_position(200, 200);
    world.run_system<app::Renderer::init>();
    world.resource<EventBus>().publish<ui_protocol::WindowResizeEvent>(
        kWindowWidth, kWindowHeight);
    world.run_system<run>();
    return 0;
}