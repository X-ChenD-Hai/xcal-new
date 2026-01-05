#include "./draw_driver.hpp"

#include <GLFW/glfw3.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_node_editor.h>

#include <ecs/world.hpp>
#include <print>

#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "glbinding/gl/functions.h"
// 顶点着色器源码
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        ourColor = aColor;
    }
)";

// 片段着色器源码
const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(ourColor, 1.0);
    }
)";

void DrawDriver::init(ecs::World& world) { world.add_resource<DrawDriver>(); }
DrawDriver::DrawDriver() {
    // 编译着色器
    vertex_shader_ = gl::glCreateShader(gl::GL_VERTEX_SHADER);
    gl::glShaderSource(vertex_shader_, 1, &vertexShaderSource, NULL);
    gl::glCompileShader(vertex_shader_);
    std::println("id: {}", vertex_shader_);

    fragment_shader_ = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);
    gl::glShaderSource(fragment_shader_, 1, &fragmentShaderSource, NULL);
    gl::glCompileShader(fragment_shader_);
    std::println("id: {}", fragment_shader_);

    // 创建着色器程序
    shader_program_ = gl::glCreateProgram();
    gl::glAttachShader(shader_program_, vertex_shader_);
    gl::glAttachShader(shader_program_, fragment_shader_);
    gl::glLinkProgram(shader_program_);
    std::println("id: {}", shader_program_);

    // 设置顶点数据
    float vertices[] = {
        // 位置              // 颜色
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下角 - 红色
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下角 - 绿色
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f   // 顶部 - 蓝色
    };

    // 生成VAO和VBO
    gl::glGenVertexArrays(1, &vao_);
    gl::glGenBuffers(1, &vbo_);

    // 绑定VAO
    gl::glBindVertexArray(vao_);

    // 绑定VBO并设置顶点数据
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glBufferData(gl::GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     gl::GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性
    gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE,
                              6 * sizeof(float), (void*)0);
    gl::glEnableVertexAttribArray(0);
    // 颜色属性
    gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE,
                              6 * sizeof(float), (void*)(3 * sizeof(float)));
    gl::glEnableVertexAttribArray(1);

    // 解绑
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    gl::glBindVertexArray(0);
}

DrawDriver::~DrawDriver() {
    gl::glDeleteVertexArrays(1, &vao_);
    gl::glDeleteBuffers(1, &vbo_);
    gl::glDeleteProgram(shader_program_);
    gl::glDeleteShader(vertex_shader_);
    gl::glDeleteShader(fragment_shader_);
}

void DrawDriver::update() {
    // 使用着色器程序
    gl::glUseProgram(shader_program_);

    // 绑定VAO
    gl::glBindVertexArray(vao_);

    // 绘制三角形
    gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);

    // 解绑
    gl::glBindVertexArray(0);
}
