#pragma once

#include <cstdint>

#include "./types.hpp"
namespace xc::opengl {

class VertexArray {
   public:
    VertexArray(const VertexArray&) = delete;
    VertexArray(VertexArray&&) = default;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray& operator=(VertexArray&&) = default;

   public:
    VertexArray();
    ~VertexArray();

    void bind() const noexcept;
    static void unbind() noexcept;

    void set_attribute(uint32_t index, int32_t size, DataType type,
                       bool normalized, int32_t stride,
                       const void* offset) const noexcept;

    void enable_attribute(uint32_t index) const noexcept;
    void disable_attribute(uint32_t index) const noexcept;

    vertex_array_id_t id() const noexcept { return id_; }

   private:
    vertex_array_id_t id_;
};

}  // namespace xc::opengl
