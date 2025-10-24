#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <xcmath/xcmath.hpp>
namespace xc::xcal::render::opengl {

enum class ShaderType : uint32_t {
    VERTEX,
    FRAGMENT,
    GEOMETRY,
    COMPUTE,
};
struct SingleColorShaderComponent {
    xcmath::vec4f color;
};
struct LinearGradientShaderComponent {
    xcmath::vec3f start_pos;
    xcmath::vec3f end_pos;
    xcmath::vec4f start_color;
    xcmath::vec4f end_color;
};
struct RadialGradientShaderComponent {
    xcmath::vec3f center;
};
struct VertexColorShaderComponent {};
struct Texture2dShaderComponent {};
struct Texture3dShaderComponent {};

using ShaderComponent =
    std::variant<SingleColorShaderComponent, LinearGradientShaderComponent,
                 RadialGradientShaderComponent, VertexColorShaderComponent,
                 Texture2dShaderComponent, Texture3dShaderComponent>;

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