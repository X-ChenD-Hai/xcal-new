#include "./Mesh.hpp"

#include <ecs/World.hpp>
#include <xcal/transform/transform.hpp>

#include "./Shader.hpp"
#include "./openglloader.h"

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
                _gl glDrawArrays(_gl GL_TRIANGLES, m.draw_offset, m.draw_count);
            else {
                _gl glDrawElements(_gl GL_TRIANGLES, m.draw_count,
                                   _gl GL_UNSIGNED_INT,
                                   (const void *)((size_t)m.draw_offset));
            }
        });
}
