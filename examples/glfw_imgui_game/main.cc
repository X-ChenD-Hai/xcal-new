#include <glbinding/gl/gl.h>
#include <imgui.h>
#include <imgui_node_editor.h>

//
#include <cstdint>
#include <print>
#include <xc/ecs/command/attach_components.hpp>
#include <xc/ecs/entity.hpp>
#include <xc/ecs/world.hpp>

#include "./app_context.hpp"
#include "./draw_driver.hpp"
#include "glbinding/gl/functions.h"
#include "node_editor.hpp"

namespace render::gl {
ecs::Entity create_object(ecs::World& world) {
    auto e = world.create_entity();
    uint32_t a;
    ::gl::glGenVertexArrays(1, &a);
    world.submit().submit<ecs::command::AttachComponents>(e, VAO{a});
    return e;
};

void attach_vbo(ecs::Entity e);

}  // namespace render::gl

struct WindowHandle {
    static void init(ecs::World& world) { world.add_resource<WindowHandle>(); }
    void update(ecs::World& world) {}

   public:
    bool show{true};
};

class App {
   public:
    static void init(ecs::World& world) {
        world.use_plugin<AppContext>()
            .run_system<WindowHandle::init>()
            .run_system<DrawDriver::init>()
            .run_system<::NodeEditor::init>()
            .add_resource<App>();
    }
    void run(ecs::World& world, const AppContext& context, ecs::EventBus& bus) {
        running_flag = true;
        gl::glClearColor(0.3, 0.3, 0.3, 1);
        while (!bus.exist<AppContext::CloseRequestEvent>()) {
            context.begin_frame();
            gl::glClear(gl::GL_COLOR_BUFFER_BIT);
            world.run_system<&DrawDriver::update>()
                .run_system<&WindowHandle::update>()
                .run_system<&::NodeEditor::update>();
            bus.each([](AppContext::FrameResizeEvent& e) {
                gl::glViewport(0, 0, e.width, e.height);
                std::println("FrameResizeEvent");
            });

            bus.clear();
            context.end_frame(bus);
        }
    }

   private:
    bool running_flag{false};
};

int main() {
    ecs::World world;

    world.run_system<App::init>().run_system<&App::run>();

    return 0;
}
