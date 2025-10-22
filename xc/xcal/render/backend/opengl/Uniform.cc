#include "./uniform.hpp"

#include "./openglloader.h"

xc::xcal::render::opengl::UniformBuffer::UniformBuffer() {
    _gl glGenBuffers(1, &ubo);
    _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
    _gl glBufferData(_gl GL_UNIFORM_BUFFER, sizeof(xcmath::mat4f),
                     &xcmath::mat4f::eye()[0][0], _gl GL_DYNAMIC_DRAW);
}
void xc::xcal::render::opengl::UniformBuffer::update(const xcmath::mat4f& mat) {
    _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
    _gl glBufferSubData(_gl GL_UNIFORM_BUFFER, 0, sizeof(xcmath::mat4f),
                        &mat[0][0]);
    _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, 0);
}
void xc::xcal::render::opengl::UniformBuffer::bind() {
    _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
    _gl glBindBufferBase(_gl GL_UNIFORM_BUFFER, 1, ubo);
    // _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, 0);
}
