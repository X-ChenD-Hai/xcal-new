#include "./shader_source.hpp"

#include <print>

#include "./opengl_api.hpp"
namespace xc::opengl {

ShaderSource::ShaderSource(const ShaderSourceDescriptor& desc) {
    shader_id_ = glCreateShader(enum_to_gl(desc.type));

    std::string source(desc.source);
    const char* src = source.c_str();

    glShaderSource(shader_id_, 1, &src, nullptr);
    glCompileShader(shader_id_);

    int success;
    glGetShaderiv(shader_id_, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length;
        glGetShaderiv(shader_id_, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '0');
        glGetShaderInfoLog(shader_id_, length, &length, &log[0]);
        std::print("Shader compile error({}) : {}\n", shader_id_, log);
    } else {
        std::println("Shader compile success: {}", shader_id_);
    }
}
ShaderSource::~ShaderSource() { glDeleteShader(shader_id_); }
}  // namespace xc::opengl
