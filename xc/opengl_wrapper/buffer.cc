#include "./buffer.hpp"

#include "./opengl_api.hpp"

xc::opengl::Buffer::Buffer() : id_{0} { glGenBuffers(1, &id_); }
xc::opengl::Buffer::~Buffer() {
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
    }
}

void xc::opengl::Buffer::bind(BufferTarget type) const noexcept {
    glBindBuffer(enum_to_gl(type), id_);
}

void xc::opengl::Buffer::unbind(BufferTarget type) noexcept {
    glBindBuffer(enum_to_gl(type), 0);
}

void xc::opengl::Buffer::buffer_data(void* data, size_t size,
                                     BufferTarget target,
                                     BufferUsage usage) const noexcept {
    glBufferData(enum_to_gl(target), static_cast<GLsizeiptr>(size), data,
                 enum_to_gl(usage));
}

void xc::opengl::Buffer::read(void* data, size_t size,
                              BufferTarget target) const noexcept {
    glGetBufferSubData(enum_to_gl(target), 0, static_cast<GLsizeiptr>(size),
                       data);
}
void xc::opengl::Buffer::buffer_sub_data(void* data, size_t size,
                                         BufferTarget target,
                                         BufferUsage usage) const noexcept {
    glBufferSubData(enum_to_gl(target), 0, static_cast<GLsizeiptr>(size), data);
}
