#include <glbinding/gl/gl.h>
#include <imgui.h>

#include <cstdint>
#include <print>

#include "./app_context.hpp"
#include "ecs/command/attach_components.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "glbinding/gl/functions.h"
using namespace gl;

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

struct VAO {
    uint32_t vao;
};
struct VBO {
    uint32_t vbo;
    uint32_t count;
};

struct EBO {
    uint32_t vbo;
    uint32_t ebo;
    uint32_t count;
};

struct UBO {
    uint32_t ubo;
};

struct BufferLayout {};

class Shader {};

namespace render::gl {
ecs::Entity create_object(ecs::World& world) {
    auto e = world.create_entity();
    uint32_t a;
    glGenVertexArrays(1, &a);
    world.submit().submit<ecs::command::AttachComponents>(e, VAO{a});
    return e;
};

void attach_vbo(ecs::Entity e);

}  // namespace render::gl

struct WindowHandle {
    static void init(ecs::World& world) { world.add_resource<WindowHandle>(); }
    void update(ecs::World& world) {
        if (!show) return;
        using namespace ImGui;
        Begin("aa", &show);
        SetWindowFontScale(2.f);
        Text("Hello world");
        End();
    }

   public:
    bool show{true};
};

struct DrawDriver {
    static void init(ecs::World& world) { world.add_resource<DrawDriver>(); }
    DrawDriver() {
        // 编译着色器
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // 创建着色器程序
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // 设置顶点数据
        float vertices[] = {
            // 位置              // 颜色
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下角 - 红色
            0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下角 - 绿色
            0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f   // 顶部 - 蓝色
        };

        // 生成VAO和VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // 绑定VAO
        glBindVertexArray(VAO);

        // 绑定VBO并设置顶点数据
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);

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

    ~DrawDriver() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void update() {
        // 使用着色器程序
        glUseProgram(shaderProgram);

        // 绑定VAO
        glBindVertexArray(VAO);

        // 绘制三角形
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 解绑
        glBindVertexArray(0);
    }

   private:
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int shaderProgram;
    unsigned int VAO, VBO;
};

class App {
   public:
    static void init(ecs::World& world) {
        world.use_plugin<AppContext>()
            .run_system<WindowHandle::init>()
            .run_system<DrawDriver::init>()
            .add_resource<App>();
    }
    void run(ecs::World& world, const AppContext& context, ecs::EventBus& bus) {
        running_flag = true;
        glClearColor(0.3, 0.3, 0.3, 1);
        while (!bus.exist<AppContext::CloseRequestEvent>()) {
            context.begin_frame();
            glClear(GL_COLOR_BUFFER_BIT);
            world.run_system<&DrawDriver::update>()
                .run_system<&WindowHandle::update>();
            bus.each([](AppContext::FrameResizeEvent& e) {
                glViewport(0, 0, e.width, e.height);
                std::println("FrameResizeEvent");
            });

            bus.clear();
            context.end_frame(bus);
        }
    }

   private:
    bool running_flag{false};
};

int main() {
    ecs::World world;

    world.run_system<App::init>().run_system<&App::run>();

    return 0;
}
