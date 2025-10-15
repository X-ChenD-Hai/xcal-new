#include "./Shader.hpp"

#include <fstream>
#include <print>

#include "./openglloader.h"
xc::xcal::render::opengl::Shader::Shader(const std::string& vertex_shader,
                                         const std::string& fragment_shader)
    : vertex_shader_path(vertex_shader), fragment_shader_path(fragment_shader) {
    // 创建普通的着色器程序，而不是程序管线
    program = _gl glCreateProgram();

    auto vertex_shader_obj =
        create_shader(vertex_shader_path, ShaderType::VERTEX);
    auto fragment_shader_obj =
        create_shader(fragment_shader_path, ShaderType::FRAGMENT);

    _gl glAttachShader(program, vertex_shader_obj);
    _gl glAttachShader(program, fragment_shader_obj);
    _gl glLinkProgram(program);

    int success;
    _gl glGetProgramiv(program, _gl GL_LINK_STATUS, &success);
    if (!success) {
        int length;
        _gl glGetProgramiv(program, _gl GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '0');
        _gl glGetProgramInfoLog(program, length, &length, &log[0]);
        std::print("Shader link error: {}\n", log);
    }

    _gl glValidateProgram(program);
    int validate_success;
    _gl glGetProgramiv(program, _gl GL_VALIDATE_STATUS, &validate_success);
    if (!validate_success) {
        int length;
        _gl glGetProgramiv(program, _gl GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '0');
        _gl glGetProgramInfoLog(program, length, &length, &log[0]);
        std::print("Shader validate error: {}\n", log);
    }

    // 清理着色器对象
    _gl glDeleteShader(vertex_shader_obj);
    _gl glDeleteShader(fragment_shader_obj);

    std::println("Shader init success: {}", program);
}
uint32_t xc::xcal::render::opengl::Shader::create_shader(
    const std::string& path, ShaderType shader_type) {
    static constexpr _gl GLenum shader_type_map[] = {
        _gl GL_VERTEX_SHADER,
        _gl GL_FRAGMENT_SHADER,
        _gl GL_GEOMETRY_SHADER,
        _gl GL_COMPUTE_SHADER,
    };

    _gl GLuint shader =
        _gl glCreateShader(shader_type_map[static_cast<uint32_t>(shader_type)]);

    std::ifstream file(path);
    if (!file.is_open()) {
        std::print("Shader file not found: {}\n", path);
        return 0;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                       (std::istreambuf_iterator<char>()));
    const char* src = source.c_str();

    _gl glShaderSource(shader, 1, &src, nullptr);
    _gl glCompileShader(shader);

    int success;
    _gl glGetShaderiv(shader, _gl GL_COMPILE_STATUS, &success);
    if (!success) {
        int length;
        _gl glGetShaderiv(shader, _gl GL_INFO_LOG_LENGTH, &length);
        std::string log(length, '0');
        _gl glGetShaderInfoLog(shader, length, &length, &log[0]);
        std::print("Shader compile error ({}): {}\n", path, log);
    } else {
        std::println("Shader compile success: {}", path);
    }

    return shader;
}
void xc::xcal::render::opengl::Shader::use() { _gl glUseProgram(program); }
xc::xcal::render::opengl::Shader::~Shader() {
    std::println("Shader destroy: {}", program);
    _gl glDeleteProgram(program);
}
