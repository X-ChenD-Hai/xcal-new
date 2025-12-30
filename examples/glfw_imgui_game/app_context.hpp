#pragma once

#include "ecs/event_bus.hpp"
#include "ecs/world.hpp"

struct GLFWwindow;

class AppContext {
    friend class ecs::World;

   public:
    struct CloseRequestEvent {};
    struct FrameResizeEvent {
        float width;
        float height;
    };
    ~AppContext() { cleanup(); }

    static AppContext* install(ecs::World& world);
    static void uninstall(ecs::World& world, AppContext* p);

    void begin_frame() const;

    void end_frame(ecs::EventBus& bus) const;

   protected:
    AppContext(ecs::World& world) : window_(nullptr), world_(world) {}

    bool initialize();

    bool init_glfw();
    void load_opengl();
    bool init_glfw_window();

    bool init_imgui();

    void cleanup();

   private:
    GLFWwindow* window_;
    ecs::World& world_;
};
