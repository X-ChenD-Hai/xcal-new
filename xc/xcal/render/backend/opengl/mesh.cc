#include "./mesh.hpp"

#include <xc/ecs/resource_table.hpp>
#include <xc/ecs/world.hpp>
#include <xc/xcal/transform/transform.hpp>

#include "./openglloader.h"
#include "./shader.hpp"

static constexpr _gl GLenum MeshType2Glenum[] = {
    _gl GL_LINE_STRIP,      // LINE_STRIP
    _gl GL_LINE_LOOP,       // LINE_LOOP
    _gl GL_LINES,           // LINES
    _gl GL_POINTS,          // POINTS
    _gl GL_TRIANGLES,       // TRIANGLES
    _gl GL_TRIANGLE_STRIP,  // TRIANGLE_STRIP
    _gl GL_TRIANGLE_FAN,    // TRIANGLE_FAN
    _gl GL_QUADS,           // QUADS
    _gl GL_QUAD_STRIP,      // QUAD_STRIP
    _gl GL_POLYGON,         // POLYGON
};
static constexpr _gl GLenum VertexType2Glenum[] = {
    _gl GL_FLOAT,
    _gl GL_DOUBLE,
    _gl GL_INT,
    _gl GL_UNSIGNED_INT,
};
void xc::xcal::render::opengl::VertexLayout::dump() const {
    for (size_t i = 0; i < attributes.size(); ++i) {
        attributes[i].dump(i);
    }
}
void xc::xcal::render::opengl::VertexAttribute::dump(uint32_t location) const {
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_id);
    _gl glVertexAttribPointer(location, size, VertexType2Glenum[(uint8_t)type],
                              normalized ? _gl GL_TRUE : _gl GL_FALSE, stride,
                              (void*)((size_t)offset));
    _gl glEnableVertexAttribArray(location);
}

xc::xcal::render::opengl::VertexArrayObject::VertexArrayObject(
    const VertexLayout& layout, uint32_t ebo) {
    _gl glGenVertexArrays(1, &id);
    _gl glBindVertexArray(id);
    layout.dump();
    _gl glBindBuffer(_gl GL_ELEMENT_ARRAY_BUFFER, ebo);
    std::print("VAO init {}\n", id);
}

void xc::xcal::render::opengl::render_mesh(ecs::Querier q,
                                           ecs::ComponentAccessor a,
                                           ecs::ResourceTable& resources) {
    using namespace xc::xcal;
    auto shader = resources.create_or_get<Shader, SingleColorShaderComponent>(
        "res/single_color.vs", "res/single_color.fs");
    _gl glUseProgram(shader->program);
    auto model_location = _gl glGetUniformLocation(shader->program, "model");
    auto color_location = _gl glGetUniformLocation(shader->program, "color");
    // std::println("start render------------");
    a.each<transform::TransformMatrixComponent, SingleColorShaderComponent,
           MeshComponent>(
        [](auto& t, auto& s, auto& m, auto model_location,
           auto color_location) {
            _gl glUniformMatrix4fv(model_location, 1, _gl GL_FALSE,
                                   &t.matrix.T()[0][0]);
            _gl glUniform4fv(color_location, 1, &s.color[0]);
            _gl glBindVertexArray(m.vao_id);
            if (m.use_element_buffer) {
                // std::println("draw element type {} count {}, offset {}",
                // m.type,
                //              m.draw_count, m.draw_offset);
                _gl glDrawElements(MeshType2Glenum[(uint32_t)m.type],
                                   m.draw_count, _gl GL_UNSIGNED_INT,
                                   (const void*)((size_t)m.draw_offset));
            } else {
                // std::println("draw array type {} count {}, offset {}",
                // m.type,
                //              m.draw_count, m.draw_offset);
                _gl glDrawArrays(MeshType2Glenum[(uint32_t)m.type],
                                 m.draw_offset, m.draw_count);
            }
        },
        model_location, color_location);
    shader = resources.create_or_get<Shader, VertexColorShaderComponent>(
        "res/vertex_color.vs", "res/vertex_color.fs");
    _gl glUseProgram(shader->program);
    model_location = _gl glGetUniformLocation(shader->program, "model");
    a.each<transform::TransformMatrixComponent, VertexColorShaderComponent,
           MeshComponent>(
        [](auto& t, auto& s, auto& m, auto model_location) {
            _gl glUniformMatrix4fv(model_location, 1, _gl GL_FALSE,
                                   &t.matrix.T()[0][0]);
            _gl glBindVertexArray(m.vao_id);
            if (m.use_element_buffer) {
                // std::println("draw element type {} count {}, offset {}",
                // m.type,
                //              m.draw_count, m.draw_offset);
                _gl glDrawElements(MeshType2Glenum[(uint32_t)m.type],
                                   m.draw_count, _gl GL_UNSIGNED_INT,
                                   (const void*)((size_t)m.draw_offset));
            } else {
                // std::println("draw array type {} count {}, offset {}",
                // m.type,
                //              m.draw_count, m.draw_offset);
                _gl glDrawArrays(MeshType2Glenum[(uint32_t)m.type],
                                 m.draw_offset, m.draw_count);
            }
        },
        model_location);
}
xc::xcal::render::opengl::MeshComponent
xc::xcal::render::opengl::Mesh::mesh_component() const {
    auto id = VertexArrayObject(layout(), ebo()).id;
    std::print("VAO {} for mesh\n", id);
    return {
        .vao_id = id,
        .use_element_buffer = ebo() != 0,
        .type = draw_type(),
        .draw_count = draw_count(),
        .draw_offset = draw_offset(),
    };
}
void xc::xcal::render::opengl::Mesh::set_color(const xcmath::vec4f& color) {
    shader_program = SingleColorShaderComponent{color};
}
void xc::xcal::render::opengl::Mesh::use_vertex_color() {
    shader_program = VertexColorShaderComponent{};
};