#include "./Event.hpp"

#include <print>
#include <xcal/camera/Camera.hpp>
#include <xcal/event/events.hpp>

#include "./Uniform.hpp"
#include "./openglloader.h"

void xc::xcal::render::opengl::handle_event(ecs::ResourceManager& resources,
                                            ecs::EventBus& bus,
                                            ecs::ResourceTable& table) {
    bus.each<xcal::event::FrameResize>([](auto& e) {
        std::println("frame resize");
        _gl glViewport(0, 0, e.width, e.height);
    });
    if (bus.any_exist<event::CameraProjectionChanged,
                      event::CameraViewChanged>()) {
        // std::println("camera view changed");
        auto uniform =
            table.create_or_get<xcal::render::opengl::UniformBuffer>();
        uniform->update((resources.get<camera::ProjectionConfig>().as_mat4() ^
                         resources.get<camera::ViewConfig>().as_mat4())
                            .T());
        uniform->bind();
    }
}
