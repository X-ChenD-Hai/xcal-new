#pragma once

#include <cstdint>
#include <string>
namespace xc::xcal::render::opengl {

enum class ShaderType : uint32_t {
    VERTEX,
    FRAGMENT,
    GEOMETRY,
    COMPUTE,
};
struct ShaderComponent {
    uint32_t program_id;
};
struct Shader {
    std::string vertex_shader_path, fragment_shader_path;
    uint32_t program;
    Shader(const std::string& vertex_shader,
           const std::string& fragment_shader);
    static uint32_t create_shader(const std::string& path,
                                  ShaderType shader_type);
    void use();
    ~Shader();
};
}  // namespace xc::xcal::render::opengl