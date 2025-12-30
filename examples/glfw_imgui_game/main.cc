#include <glbinding/gl/gl.h>
#include <imgui.h>

#include "./app_context.hpp"
#include "ecs/world.hpp"
using namespace gl;
struct WindowHandle {
    static void init(ecs::World& world) { world.add_resource<WindowHandle>(); }
    void update(ecs::World& world) {
        if (!show) return;
        using namespace ImGui;
        Begin("aa", &show);
        SetWindowFontScale(2.f);
        Text("Hello world");
        End();
    }

   public:
    bool show{true};
};

struct DrawDriver {
    static void init(ecs::World& world) { world.add_resource<DrawDriver>(); }
    void update() {}
};

class App {
   public:
    static void init(ecs::World& world) {
        world.use_plugin<AppContext>()
            .run_system<WindowHandle::init>()
            .run_system<DrawDriver::init>()
            .add_resource<App>();
    }
    void run(ecs::World& world, const AppContext& context, ecs::EventBus& bus) {
        running_flag = true;
        glClearColor(0.3, 0.3, 0.3, 1);
        while (!bus.exist<AppContext::CloseRequestEvent>()) {
            context.begin_frame();
            glClear(GL_COLOR_BUFFER_BIT);
            world.run_system<&DrawDriver::update>()
                .run_system<&WindowHandle::update>();
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
