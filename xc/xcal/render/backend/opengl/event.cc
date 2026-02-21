#include "./event.hpp"

#include <xc/ecs/command/attach_components.hpp>
#include <xc/ecs/command_submit.hpp>
#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/resource_table.hpp>
#include <xc/ecs/world.hpp>
#include <print>
#include <xc/xcal/camera/camera.hpp>
#include <xc/xcal/event/events.hpp>
#include <xc/xcal/object/line.hpp>
#include <xc/xcal/object/object.hpp>
#include <xc/xcal/transform/transform.hpp>

#include "./mesh.hpp"
#include "./mesh/axis.hpp"
#include "./openglloader.h"
#include "./uniform.hpp"

static void add_mesh(ecs::CommandSubmit& submit, ecs::Entity entity,
                     const xc::xcal::render::opengl::Mesh& mesh) {
    auto mesh_comp = mesh.mesh_component();
    xc::xcal::transform::TransformComponent transform;
    std::visit(
        [&](auto&& shader) {
            std::println("create attach command to {} ", entity.id());
            submit.submit<ecs::command::AttachComponents>(
                entity, transform, mesh_comp, shader,
                xc::xcal::transform::TransformMatrixComponent{});
            std::println("ok");
        },
        mesh.shader_program);
}
void xc::xcal::render::opengl::handle_event(ecs::ResourceManager& resources,
                                            ecs::EventBus& bus,
                                            ecs::ResourceTable& table,
                                            ecs::CommandSubmit& submit) {
    bus.each([](xcal::event::FrameResize& e) {
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
        [](object::CreateObject<object::Line>& e, ecs::CommandSubmit& submit) {
            std::println("create line id {} direction {}", e.entity.id(),
                         e.config.direction);
            add_mesh(submit, e.entity, Line{e.config.direction});
        },
        submit);
}
