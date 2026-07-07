#pragma once
#include <memory>
#include <xc/ecs/tyoes.hpp>
#include <xc/opengl_wrapper/shader_program.hpp>

namespace xcal_opengl_render {
using shader_ptr = std::unique_ptr<xc::opengl::ShaderProgram>;
struct ShaderTable {
    shader_ptr pos_then_uniform_color_shader{};
    shader_ptr pos_color_shader{};
    ShaderTable();
    void update(ecs::EventBus& bus, ecs::World& world);
};
shader_ptr make_shader(const char* vertex_shader_source,
                       const char* fragment_shader_source);
}  // namespace xcal_opengl_render