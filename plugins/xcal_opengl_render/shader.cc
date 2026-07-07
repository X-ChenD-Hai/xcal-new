#include "./shader.hpp"

#include <print>
#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/world.hpp>
#include <xc/opengl_wrapper/shader_program.hpp>
#include <xcal2/camera/camera.hpp>
#include <xcal2/events/events.hpp>

#include "./static_shader_source.hpp"

xcal_opengl_render::ShaderTable::ShaderTable() {
    using namespace xc::opengl;
    pos_then_uniform_color_shader = make_shader(
        static_shader_source::kVertexWithPosUniformColorShaderSource,
        static_shader_source::kFragmentShaderSource);
    pos_color_shader =
        make_shader(static_shader_source::kVertexWithPosColorShaderSource,
                    static_shader_source::kFragmentShaderSource);
}
xcal_opengl_render::shader_ptr xcal_opengl_render::make_shader(
    const char* vertex_shader_source, const char* fragment_shader_source) {
    using namespace xc::opengl;
    try {
        auto shader = std::unique_ptr<ShaderProgram>(new ShaderProgram{{
            {.source = vertex_shader_source, .type = ShaderSourceType::Vertex},
            {.source = fragment_shader_source,
             .type = ShaderSourceType::Fragment},
        }});

        std::println("create shader program {}", shader->id());
        return shader;
    } catch (const std::exception& e) {
        std::println("create shader program failed: {}", e.what());
    }
    return nullptr;
}
void xcal_opengl_render::ShaderTable::update(ecs::EventBus& event_bus,
                                             ecs::World& world) {
    if (event_bus.exist<xcal::events::CameraProjectionChanged>()) {
        auto mat =
            world.resource<xcal::camera::ProjectionConfig>().as_mat4().T();
        pos_then_uniform_color_shader->use();
        pos_then_uniform_color_shader->uniform_mat4("projection",
                                                    mat.value_ptr());
        pos_then_uniform_color_shader->use();
        pos_then_uniform_color_shader->uniform_mat4("projection",
                                                    mat.value_ptr());
    }
    if (event_bus.exist<xcal::events::CameraViewChanged>()) {
        auto mat = world.resource<xcal::camera::ViewConfig>().as_mat4().T();
        pos_then_uniform_color_shader->use();
        pos_then_uniform_color_shader->uniform_mat4("view", mat.value_ptr());
        pos_color_shader->use();
        pos_color_shader->uniform_mat4("view", mat.value_ptr());
    }
}
