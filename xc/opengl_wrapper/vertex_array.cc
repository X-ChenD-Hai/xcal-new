#include "./vertex_array.hpp"

#include "./opengl_api.hpp"

namespace xc::opengl {

VertexArray::VertexArray() : id_{0} {  glGenVertexArrays(1, &id_); }

VertexArray::~VertexArray() {
    if (id_ != 0) {
         glDeleteVertexArrays(1, &id_);
    }
}

void VertexArray::bind() const noexcept {  glBindVertexArray(id_); }

void VertexArray::unbind() noexcept {  glBindVertexArray(0); }

void VertexArray::set_attribute(uint32_t index, int32_t size, DataType type,
                                bool normalized, int32_t stride,
                                const void* offset) const noexcept {
     glVertexAttribPointer(index, size, enum_to_gl(type),
                              static_cast< GLboolean>(normalized), stride,
                              offset);
}

void VertexArray::enable_attribute(uint32_t index) const noexcept {
     glEnableVertexAttribArray(index);
}

void VertexArray::disable_attribute(uint32_t index) const noexcept {
     glDisableVertexAttribArray(index);
}

}  // namespace xc::opengl
