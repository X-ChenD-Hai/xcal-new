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
using namespace gl;
using namespace ax;
using namespace ax::NodeEditor;

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
        ImGui::Begin("aa", &show);
        SetWindowFontScale(2.f);
        Text("Hello world");
        ImGui::End();
    }

   public:
    bool show{true};
};

struct MyNodeEditor {
    static void init(ecs::World& world) { world.add_resource<MyNodeEditor>(); }

    MyNodeEditor() {
        // 创建节点编辑器上下文
        m_EditorContext = ax::NodeEditor::CreateEditor();

        // 设置节点和连接的初始状态
        m_NextNodeId = 100;
        m_NextLinkId = 1000;
    }

    ~MyNodeEditor() {
        // 销毁节点编辑器上下文
        ax::NodeEditor::DestroyEditor(m_EditorContext);
    }

    void update(ecs::World& world) {
        using namespace ImGui;

        // 设置当前编辑器上下文
        ax::NodeEditor::SetCurrentEditor(m_EditorContext);

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
        for (auto& node : m_Nodes) {
            ax::NodeEditor::BeginNode(node.id);

            // 节点标题
            ImGui::Text("Node %llu",
                        reinterpret_cast<uintptr_t>(node.id.AsPointer()));

            // 输入引脚
            ax::NodeEditor::PinId inputPinId =
                ax::NodeEditor::PinId{reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(node.id.AsPointer()) + 1)};
            ax::NodeEditor::BeginPin(inputPinId,
                                     ax::NodeEditor::PinKind::Input);
            ImGui::Text("Input");
            ax::NodeEditor::EndPin();

            // 节点内容
            ImGui::BeginGroup();
            ImGui::Text("Node Content");
            ImGui::EndGroup();

            // 输出引脚
            ax::NodeEditor::PinId outputPinId =
                ax::NodeEditor::PinId{reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(node.id.AsPointer()) + 2)};
            ax::NodeEditor::BeginPin(outputPinId,
                                     ax::NodeEditor::PinKind::Output);
            ImGui::Text("Output");
            ax::NodeEditor::EndPin();

            ax::NodeEditor::EndNode();
        }

        // 创建连接
        for (auto& link : m_Links) {
            ax::NodeEditor::Link(link.id, link.inputPinId, link.outputPinId);
        }

        // 处理新的连接
        HandleCreateLink();

        // 处理删除连接
        HandleDeleteLink();
    }

    void HandleCreateLink() {
        // 开始连接
        if (ax::NodeEditor::BeginCreate()) {
            ax::NodeEditor::PinId inputPinId, outputPinId;
            if (ax::NodeEditor::QueryNewLink(&inputPinId, &outputPinId)) {
                // 验证连接
                if (inputPinId && outputPinId) {
                    // 接受连接
                    ax::NodeEditor::AcceptNewItem(ImColor(255, 255, 255));

                    // 添加新连接
                    ax::NodeEditor::LinkId newLinkId = ax::NodeEditor::LinkId{
                        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(
                                                    m_NextLinkId.AsPointer()) +
                                                1)};
                    m_Links.push_back({newLinkId, inputPinId, outputPinId});
                    m_NextLinkId = newLinkId;
                }
            }
        }
        ax::NodeEditor::EndCreate();
    }

    void HandleDeleteLink() {
        // 处理删除连接
        if (ax::NodeEditor::BeginDelete()) {
            ax::NodeEditor::LinkId deletedLinkId;
            while (ax::NodeEditor::QueryDeletedLink(&deletedLinkId)) {
                // 从连接列表中删除
                m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
                                             [deletedLinkId](const Link& link) {
                                                 return link.id ==
                                                        deletedLinkId;
                                             }),
                              m_Links.end());
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
        ax::NodeEditor::PinId inputPinId;
        ax::NodeEditor::PinId outputPinId;
    };

   public:
    bool show{true};

   private:
    ax::NodeEditor::EditorContext* m_EditorContext{nullptr};

    std::vector<Node> m_Nodes{{ax::NodeEditor::NodeId{reinterpret_cast<void*>(
                                   static_cast<uintptr_t>(1))},
                               ImVec2(100, 100)},
                              {ax::NodeEditor::NodeId{reinterpret_cast<void*>(
                                   static_cast<uintptr_t>(2))},
                               ImVec2(400, 200)}};

    std::vector<Link> m_Links{
        {ax::NodeEditor::LinkId{
             reinterpret_cast<void*>(static_cast<uintptr_t>(100))},
         ax::NodeEditor::PinId{
             reinterpret_cast<void*>(static_cast<uintptr_t>(2))},
         ax::NodeEditor::PinId{reinterpret_cast<void*>(
             static_cast<uintptr_t>(3))}}  // 连接节点1的输出到节点2的输入
    };

    ax::NodeEditor::NodeId m_NextNodeId{
        reinterpret_cast<void*>(static_cast<uintptr_t>(100))};
    ax::NodeEditor::LinkId m_NextLinkId{
        reinterpret_cast<void*>(static_cast<uintptr_t>(1000))};
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
            .run_system<MyNodeEditor::init>()
            .add_resource<App>();
    }
    void run(ecs::World& world, const AppContext& context, ecs::EventBus& bus) {
        running_flag = true;
        glClearColor(0.3, 0.3, 0.3, 1);
        while (!bus.exist<AppContext::CloseRequestEvent>()) {
            context.begin_frame();
            glClear(GL_COLOR_BUFFER_BIT);
            world.run_system<&DrawDriver::update>()
                .run_system<&WindowHandle::update>()
                .run_system<&MyNodeEditor::update>();
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
