#include "./Mesh.hpp"

#include <ecs/World.hpp>
#include <xcal/transform/transform.hpp>

#include "./Shader.hpp"
#include "./openglloader.h"
static constexpr _gl GLenum MeshType2Glenum[] = {
    _gl GL_LINE_STRIP,      // LINE_STRIP
    _gl GL_LINE_LOOP,       // LINE_LOOP
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
    for (const auto &attribute : attributes) {
        attribute.dump();
    }
}
void xc::xcal::render::opengl::VertexAttribute::dump() const {
    _gl glEnableVertexAttribArray(location);
    _gl glVertexAttribPointer(location, size, VertexType2Glenum[(uint8_t)type],
                              normalized ? _gl GL_TRUE : _gl GL_FALSE, stride,
                              (void *)((size_t)offset));
}
     std::unordered_map<xc::xcal::render::opengl::VertexLayout, uint32_t> xc::xcal::render::opengl::VertexArrayObject:: vaos{};

xc::xcal::render::opengl::VertexArrayObject::   VertexArrayObject(
    const VertexLayout &layout) {
    auto it = vaos.find(layout);
    if (it != vaos.end()) {
        id = it->second;
        return;
    }
    _gl glGenVertexArrays(1, &id);
    _gl glBindVertexArray(id);
    layout.dump();
    std::print("VAO init {}\n", id);
    vaos[layout] = id;
}

void xc::xcal::render::opengl::render_mesh(ecs::Querier q,
                                           ecs::ComponentAccessor a) {
    using namespace xc::xcal;

    a.each<transform::TransformMatrixComponent, ShaderComponent, MeshComponent>(
        [](auto &t, auto &s, auto &m) {
            _gl glUseProgram(s.program_id);
            _gl glUniformMatrix4fv(
                _gl glGetUniformLocation(s.program_id, "model"), 1,
                _gl GL_FALSE, &t.matrix.T()[0][0]);
            _gl glBindVertexArray(m.vao_id);
            if (m.ebo_id == 0)
                _gl glDrawArrays(MeshType2Glenum[(uint32_t)m.type],
                                 m.draw_offset, m.draw_count);
            else {
                _gl glDrawElements(MeshType2Glenum[(uint32_t)m.type],
                                   m.draw_count, _gl GL_UNSIGNED_INT,
                                   (const void *)((size_t)m.draw_offset));
            }
        });
}
