#include "./event.hpp"

#include <ecs/command_submit.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/resource_table.hpp>
#include <ecs/world.hpp>
#include <print>
#include <xcal/camera/camera.hpp>
#include <xcal/event/events.hpp>
#include <xcal/object/line.hpp>
#include <xcal/object/object.hpp>
#include <xcal/transform/transform.hpp>

#include "./mesh.hpp"
#include "./mesh/axis.hpp"
#include "./openglloader.h"
#include "./uniform.hpp"

void xc::xcal::render::opengl::handle_event(ecs::World& world,
                                            ecs::ResourceManager& resources,
                                            ecs::EventBus& bus,
                                            ecs::ResourceTable& table,
                                            ecs::CommandSubmit& submit) {
    bus.each<xcal::event::FrameResize>([](auto& e) {
        std::println("frame resize");
        _gl glViewport(0, 0, e.width, e.height);
    });
    if (bus.any_exist<event::CameraProjectionChanged,
                      event::CameraViewChanged>()) {
        auto uniform =
            table.create_or_get<xcal::render::opengl::UniformBuffer>();
        uniform->update((resources.get<camera::ProjectionConfig>().as_mat4() ^
                         resources.get<camera::ViewConfig>().as_mat4())
                            .T());
        uniform->bind();
    }
    bus.each(
        [](object::CreateObject<object::Line>& e, ecs::World& world,
           ecs::CommandSubmit& submit) {
            std::println("create line id {} direction {}", e.entity.id(),
                         e.config.direction);
            auto mesh = Line(e.config.direction);
            auto mesh_comp = mesh.mesh_component();
            auto command = submit.submit<ecs::AttachComponents>(
                e.entity, mesh.mesh_component());
            std::visit([&](auto&& shader) { command->add(shader); },
                       mesh.shader_program);
            if (!world.component_info<transform::TransformComponent>()
                     .has_entity(e.entity)) {
                command->add<transform::TransformComponent>();
            }
            if (!world.component_info<transform::TransformMatrixComponent>()
                     .has_entity(e.entity)) {
                command->add<transform::TransformMatrixComponent>();
            }
            std::println("create attach command to {} ", e.entity.id());
            std::println("ok");
        },
        world, submit);
}
