#pragma once
#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/world.hpp>

namespace opengl_support {
using glproc = void (*)();
using GetProcAddressFunc = glproc (*)(const char*);
struct FrameResizeEvent {
    int width;
    int height;
};
class OpenGLSupport {
    friend class ::ecs::World;

   public:
    // 插件生命周期管理接口，符合World类的插件系统要求
    static OpenGLSupport* install(ecs::World& world,
                                  GetProcAddressFunc get_proc_address);

    static void uninstall(ecs::World& world, OpenGLSupport* plugin);

   private:
    // 内部实现，不对外暴露
    OpenGLSupport(ecs::World& world) : world_(world) {}
    ~OpenGLSupport() = default;

    // 初始化和清理方法，与OpenGL API隔离
    void load(GetProcAddressFunc get_proc_address);
    void cleanup();

   private:
    ecs::World& world_;
    bool initialized_{false};
};

}  // namespace opengl_support
