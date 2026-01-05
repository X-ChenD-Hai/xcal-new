#include "./shader_program.hpp"

#include <print>
#include <stdexcept>
#include <vector>

#include "./opengl_api.hpp"

xc::opengl::ShaderProgram::ShaderProgram(
    std::initializer_list<ShaderSourceDescriptor> desc) {
    program_id_ = glCreateProgram();
    std::vector<ShaderSource> sources;
    sources.reserve(desc.size());
    for (const auto& source_desc : desc) {
        sources.emplace_back(source_desc);
        glAttachShader(program_id_, sources.back().shader_id());
    }
    glLinkProgram(program_id_);
    GLint success;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetProgramInfoLog(program_id_, 1024, nullptr, info_log);
        throw std::runtime_error(
            std::string("Shader program linking failed: ") + info_log);
    }
}
void xc::opengl::ShaderProgram::use() const noexcept {
    glUseProgram(program_id_);
}
xc::opengl::ShaderProgram::~ShaderProgram() {
    std::println("ShaderProgram::~ShaderProgram() program_id_: {}",
                 program_id_);
    glDeleteProgram(program_id_);
}
void xc::opengl::ShaderProgram::uniform(const std::string& name,
                                        float value) const noexcept {
    glUniform1f(glGetUniformLocation(program_id_, name.c_str()), value);
}
void xc::opengl::ShaderProgram::uniform(const std::string& name,
                                        int value) const noexcept {
    glUniform1i(glGetUniformLocation(program_id_, name.c_str()), value);
}

void xc::opengl::ShaderProgram::uniform_vec2(const std::string& name,
                                             float* vec) const noexcept {
    glUniform2fv(glGetUniformLocation(program_id_, name.c_str()), 1,
                 (float*)vec);
}
void xc::opengl::ShaderProgram::uniform_vec3(const std::string& name,
                                             float* vec) const noexcept {
    glUniform3fv(glGetUniformLocation(program_id_, name.c_str()), 1,
                 (float*)vec);
}
void xc::opengl::ShaderProgram::uniform_vec4(const std::string& name,
                                             float* vec) const noexcept {
    glUniform4fv(glGetUniformLocation(program_id_, name.c_str()), 1,
                 (float*)vec);
}
void xc::opengl::ShaderProgram::uniform_mat3(const std::string& name,
                                             float* mat) const noexcept {
    glUniformMatrix3fv(glGetUniformLocation(program_id_, name.c_str()), 1,
                       GL_FALSE, (float*)mat);
}
void xc::opengl::ShaderProgram::uniform_mat4(const std::string& name,
                                             float* mat) const noexcept {
    glUniformMatrix4fv(glGetUniformLocation(program_id_, name.c_str()), 1,
                       GL_FALSE, (float*)mat);
}