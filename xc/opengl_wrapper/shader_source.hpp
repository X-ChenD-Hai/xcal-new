#pragma once
#include <string_view>

#include "opengl_wrapper/types.hpp"

namespace xc::opengl {

struct ShaderSourceDescriptor {
    std::string_view source;
    ShaderSourceType type;
};

class ShaderSource {
   public:
    ShaderSource(const ShaderSource&) = delete;
    ShaderSource(ShaderSource&&) = default;
    ShaderSource& operator=(const ShaderSource&) = delete;
    ShaderSource& operator=(ShaderSource&&) = default;

   public:
    ShaderSource(const ShaderSourceDescriptor& desc);
    inline shader_source_id_t shader_id() const noexcept { return shader_id_; }
    ~ShaderSource();

   private:
    shader_source_id_t shader_id_;
};

}  // namespace xc::opengl
