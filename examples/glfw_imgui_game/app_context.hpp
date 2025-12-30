#pragma once

#include "ecs/event_bus.hpp"
#include "ecs/world.hpp"

struct GLFWwindow;

class AppContext {
    friend class ecs::World;

   public:
    class CloseRequestEvent {};
    AppContext() : window_(nullptr) {}
    ~AppContext() { cleanup(); }

    static AppContext* install(ecs::World& world);
    static void uninstall(ecs::World& world, AppContext* p);

    void begin_frame() const;

    void end_frame(ecs::EventBus& bus) const;

   protected:
    bool initialize();

    bool init_glfw();
    void load_opengl();
    bool init_glfw_window();

    bool init_imgui();

    void cleanup();

   private:
    GLFWwindow* window_;
};
