#include <glbinding/gl/gl.h>
#define __gl_h_
#include "./draw_driver.hpp"
//
#include <GLFW/glfw3.h>
#include <glbinding/glbinding.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_node_editor.h>

#include <print>
#include <xc/ecs/entity.hpp>
#include <xc/ecs/world.hpp>

#include "glbinding/gl/functions.h"

using namespace ::gl;

// 顶点着色器源码
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        ourColor = aColor;
    }
)glsl";

// 片段着色器源码
const char* fragmentShaderSource = R"glsl(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(ourColor, 1.0);
    }
)glsl";

void DrawDriver::init(ecs::World& world) { world.add_resource<DrawDriver>(); }
DrawDriver::DrawDriver() {
    // 编译着色器
    vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader_);
    std::println("id: {}", vertex_shader_);

    fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader_, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment_shader_);
    std::println("id: {}", fragment_shader_);

    // 创建着色器程序
    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader_);
    glAttachShader(shader_program_, fragment_shader_);
    glLinkProgram(shader_program_);
    std::println("id: {}", shader_program_);

    // 设置顶点数据
    float vertices[] = {
        // 位置              // 颜色
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下角 - 红色
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下角 - 绿色
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f   // 顶部 - 蓝色
    };

    // 生成VAO和VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    // 绑定VAO
    glBindVertexArray(vao_);

    // 绑定VBO并设置顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

DrawDriver::~DrawDriver() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteProgram(shader_program_);
    glDeleteShader(vertex_shader_);
    glDeleteShader(fragment_shader_);
}

void DrawDriver::update() {
    // 使用着色器程序
    glUseProgram(shader_program_);

    // 绑定VAO
    glBindVertexArray(vao_);

    // 绘制三角形
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // 解绑
    glBindVertexArray(0);
}
