#pragma once
#include <string>

#include "./shader_source.hpp"

namespace xc::opengl {
class ShaderProgram {
   public:
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) = default;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram& operator=(ShaderProgram&&) = default;

   public:
    ShaderProgram(std::initializer_list<ShaderSourceDescriptor> desc);
    ~ShaderProgram();
    inline shader_source_id_t program_id() const noexcept {
        return program_id_;
    }
    void use() const noexcept;
    shader_source_id_t id() const noexcept { return program_id_; }

    void uniform(const std::string& name, const float value) const noexcept;
    void uniform(const std::string& name, const int value) const noexcept;
    void uniform_vec2(const std::string& name, const float* vec) const noexcept;
    void uniform_vec3(const std::string& name, const float* vec) const noexcept;
    void uniform_vec4(const std::string& name, const float* vec) const noexcept;
    void uniform_mat3(const std::string& name, const float* mat) const noexcept;
    void uniform_mat4(const std::string& name, const float* mat) const noexcept;

   private:
    shader_source_id_t program_id_;
};

}  // namespace xc::opengl
