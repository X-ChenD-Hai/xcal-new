#include <glbinding/gl/gl.h>
#include <imgui.h>
#include <imgui_node_editor.h>

#include <cstdint>
#include <print>

#include "./app_context.hpp"
#include "ecs/command/attach_components.hpp"
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
    ::gl::glGenVertexArrays(1, &a);
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
        ImGui::Begin("aa", &show);
        SetWindowFontScale(2.f);
        Text("Hello world");
        ImGui::End();
    }

   public:
    bool show{true};
};

struct NodeEditor {
    static void init(ecs::World& world) { world.add_resource<NodeEditor>(); }

    NodeEditor() {
        // 创建节点编辑器上下文
        editor_context_ = ax::NodeEditor::CreateEditor();

        // 设置节点和连接的初始状态
        next_node_id_ = 100;
        next_link_id_ = 1000;
    }

    ~NodeEditor() {
        // 销毁节点编辑器上下文
        ax::NodeEditor::DestroyEditor(editor_context_);
    }

    void update(ecs::World& world) {
        using namespace ImGui;

        // 设置当前编辑器上下文
        ax::NodeEditor::SetCurrentEditor(editor_context_);

        // 开始节点编辑器
        ax::NodeEditor::Begin("Node Editor");

        // 创建一些示例节点
        draw_nodes();

        // 结束节点编辑器
        ax::NodeEditor::End();

        // 重置当前编辑器上下文
        ax::NodeEditor::SetCurrentEditor(nullptr);
    }

    void draw_nodes() {
        using namespace ImGui;

        // 创建节点
        for (auto& node : nodes_) {
            ax::NodeEditor::BeginNode(node.id);

            // 节点标题
            ImGui::Text("Node %llu",
                        reinterpret_cast<uintptr_t>(node.id.AsPointer()));

            // 输入引脚
            ax::NodeEditor::PinId input_pin_id =
                ax::NodeEditor::PinId{reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(node.id.AsPointer()) + 1)};
            ax::NodeEditor::BeginPin(input_pin_id,
                                     ax::NodeEditor::PinKind::Input);
            ImGui::Text("Input");
            ax::NodeEditor::EndPin();

            // 节点内容
            ImGui::BeginGroup();
            ImGui::Text("Node Content");
            ImGui::EndGroup();

            // 输出引脚
            ax::NodeEditor::PinId output_pin_id =
                ax::NodeEditor::PinId{reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(node.id.AsPointer()) + 2)};
            ax::NodeEditor::BeginPin(output_pin_id,
                                     ax::NodeEditor::PinKind::Output);
            ImGui::Text("Output");
            ax::NodeEditor::EndPin();

            ax::NodeEditor::EndNode();
        }

        // 创建连接
        for (auto& link : links_) {
            ax::NodeEditor::Link(link.id, link.input_pin_id,
                                 link.output_pin_id);
        }

        // 处理新的连接
        handle_create_link();

        // 处理删除连接
        handle_delete_link();
    }

    void handle_create_link() {
        // 开始连接
        if (ax::NodeEditor::BeginCreate()) {
            ax::NodeEditor::PinId input_pin_id, output_pin_id;
            if (ax::NodeEditor::QueryNewLink(&input_pin_id, &output_pin_id)) {
                // 验证连接
                if (input_pin_id && output_pin_id) {
                    // 接受连接
                    ax::NodeEditor::AcceptNewItem(ImColor(255, 255, 255));

                    // 添加新连接
                    ax::NodeEditor::LinkId new_link_id = ax::NodeEditor::LinkId{
                        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(
                                                    next_link_id_.AsPointer()) +
                                                1)};
                    links_.push_back(
                        {new_link_id, input_pin_id, output_pin_id});
                    next_link_id_ = new_link_id;
                }
            }
        }
        ax::NodeEditor::EndCreate();
    }

    void handle_delete_link() {
        // 处理删除连接
        if (ax::NodeEditor::BeginDelete()) {
            ax::NodeEditor::LinkId deleted_link_id;
            while (ax::NodeEditor::QueryDeletedLink(&deleted_link_id)) {
                // 从连接列表中删除
                links_.erase(
                    std::remove_if(links_.begin(), links_.end(),
                                   [deleted_link_id](const Link& link) {
                                       return link.id == deleted_link_id;
                                   }),
                    links_.end());
            }
        }
        ax::NodeEditor::EndDelete();
    }

    // 节点定义
    struct Node {
        ax::NodeEditor::NodeId id;
        ImVec2 position;
    };

    // 连接定义
    struct Link {
        ax::NodeEditor::LinkId id;
        ax::NodeEditor::PinId input_pin_id;
        ax::NodeEditor::PinId output_pin_id;
    };

   public:
    bool show{true};

   private:
    ax::NodeEditor::EditorContext* editor_context_{nullptr};

    std::vector<Node> nodes_{{ax::NodeEditor::NodeId{reinterpret_cast<void*>(
                                  static_cast<uintptr_t>(1))},
                              ImVec2(100, 100)},
                             {ax::NodeEditor::NodeId{reinterpret_cast<void*>(
                                  static_cast<uintptr_t>(2))},
                              ImVec2(400, 200)}};

    std::vector<Link> links_{{
        {ax::NodeEditor::LinkId{
             reinterpret_cast<void*>(static_cast<uintptr_t>(100))},
         ax::NodeEditor::PinId{
             reinterpret_cast<void*>(static_cast<uintptr_t>(2))},
         ax::NodeEditor::PinId{reinterpret_cast<void*>(
             static_cast<uintptr_t>(3))}}  // 连接节点1的输出到节点2的输入
    }};

    ax::NodeEditor::NodeId next_node_id_{
        reinterpret_cast<void*>(static_cast<uintptr_t>(100))};
    ax::NodeEditor::LinkId next_link_id_{
        reinterpret_cast<void*>(static_cast<uintptr_t>(1000))};
};

struct DrawDriver {
    static void init(ecs::World& world) { world.add_resource<DrawDriver>(); }
    DrawDriver() {
        // 编译着色器
        vertex_shader_ = gl::glCreateShader(gl::GL_VERTEX_SHADER);
        gl::glShaderSource(vertex_shader_, 1, &vertexShaderSource, NULL);
        gl::glCompileShader(vertex_shader_);
        std::println("id: {}",vertex_shader_);

        fragment_shader_ = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);
        gl::glShaderSource(fragment_shader_, 1, &fragmentShaderSource, NULL);
        gl::glCompileShader(fragment_shader_);
        std::println("id: {}",fragment_shader_);

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
                                  6 * sizeof(float),
                                  (void*)(3 * sizeof(float)));
        gl::glEnableVertexAttribArray(1);

        // 解绑
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
    }

    ~DrawDriver() {
        gl::glDeleteVertexArrays(1, &vao_);
        gl::glDeleteBuffers(1, &vbo_);
        gl::glDeleteProgram(shader_program_);
        gl::glDeleteShader(vertex_shader_);
        gl::glDeleteShader(fragment_shader_);
    }

    void update() {
        // 使用着色器程序
        gl::glUseProgram(shader_program_);

        // 绑定VAO
        gl::glBindVertexArray(vao_);

        // 绘制三角形
        gl::glDrawArrays(gl::GL_TRIANGLES, 0, 3);

        // 解绑
        gl::glBindVertexArray(0);
    }

   private:
    unsigned int vertex_shader_;
    unsigned int fragment_shader_;
    unsigned int shader_program_;
    unsigned int vao_, vbo_;
};

class App {
   public:
    static void init(ecs::World& world) {
        world.use_plugin<AppContext>()
            .run_system<WindowHandle::init>()
            .run_system<DrawDriver::init>()
            .run_system<::NodeEditor::init>()
            .add_resource<App>();
    }
    void run(ecs::World& world, const AppContext& context, ecs::EventBus& bus) {
        running_flag = true;
        gl::glClearColor(0.3, 0.3, 0.3, 1);
        while (!bus.exist<AppContext::CloseRequestEvent>()) {
            context.begin_frame();
            gl::glClear(gl::GL_COLOR_BUFFER_BIT);
            world.run_system<&DrawDriver::update>()
                .run_system<&WindowHandle::update>()
                .run_system<&::NodeEditor::update>();
            bus.each([](AppContext::FrameResizeEvent& e) {
                gl::glViewport(0, 0, e.width, e.height);
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
