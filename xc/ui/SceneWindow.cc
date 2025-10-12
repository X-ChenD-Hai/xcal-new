#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <ecs/CommandSubmit.hpp>
#include <ecs/EventBus.hpp>
#include <ecs/Querier.hpp>
#include <ecs/Resource.hpp>
#include <ecs/ResourceTable.hpp>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#define _gl gl::
#endif
#ifdef USE_GLAD
#include <glad/glad.h>
#define _gl
#endif
#include <print>

#include "./SceneWindow.hpp"
#include "./systems.hpp"
struct UiEditorCacher {
    xc::xcal::TransformComponent transform_component;
};

SceneWindow::SceneWindow(const std::string& name, int width, int height,
                         int fps)
    : GlfwImguiWindow(name, width, height, fps) {
    init_();
}
SceneWindow::SceneWindow() : GlfwImguiWindow() { init_(); }

struct TrangleMesh {
    glm::vec3 a, b, c;
    _gl GLuint vao, vbo, shader;
    TrangleMesh(glm::vec3 a, glm::vec3 b, glm::vec3 c) : a(a), b(b), c(c) {
        _gl glGenVertexArrays(1, &vao);
        _gl glGenBuffers(1, &vbo);
        _gl glBindVertexArray(vao);
        std::array<float, 18> data = {
            a.x, a.y, a.z, 1.0f, 0.0f, 0.0f,  //
            b.x, b.y, b.z, 0.0f, 1.0f, 0.0f,  //
            c.x, c.y, c.z, 0.0f, 0.0f, 1.0f,  //
        };
        _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo);
        _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(float),
                         data.data(), _gl GL_STATIC_DRAW);
        _gl glEnableVertexAttribArray(0);
        _gl glVertexAttribPointer(0, 3, _gl GL_FLOAT, _gl GL_FALSE,
                                  6 * sizeof(float), (void*)0);
        _gl glEnableVertexAttribArray(1);
        _gl glVertexAttribPointer(1, 3, _gl GL_FLOAT, _gl GL_FALSE,
                                  6 * sizeof(float),
                                  (void*)(3 * sizeof(float)));
        std::print("Trangle init\n");
    }
    void draw() {
        _gl glBindVertexArray(vao);
        _gl glDrawArrays(_gl GL_TRIANGLES, 0, 3);
    }
    xc::xcal::MeshComponent mesh_component() {
        return {
            .vao_id = vao,
            .vbo_id = vbo,
            .ebo_id = 0,
            .draw_count = 3,
            .draw_offset = 0,
        };
    }
};
struct Shader {
    std::string vertex_shader_path, fragment_shader_path;
    _gl GLuint program;
    Shader(const std::string& vertex_shader, const std::string& fragment_shader)
        : vertex_shader_path(vertex_shader),
          fragment_shader_path(fragment_shader) {
        // 创建普通的着色器程序，而不是程序管线
        program = _gl glCreateProgram();

        auto vertex_shader_obj =
            create_shader(vertex_shader_path, _gl GL_VERTEX_SHADER);
        auto fragment_shader_obj =
            create_shader(fragment_shader_path, _gl GL_FRAGMENT_SHADER);

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
    static _gl GLuint create_shader(const std::string& path,
                                    _gl GLenum shader_type) {
        _gl GLuint shader = _gl glCreateShader(shader_type);

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
    void use() { _gl glUseProgram(program); }
    ~Shader() {
        std::println("Shader destroy: {}", program);
        _gl glDeleteProgram(program);
    }
};
void render_mesh(ecs::Querier q, ecs::ComponentAccessor a) {
    using namespace xc::xcal;
    // std::print("render mesh\n");
    for (auto e : q.query<TransformMatrixComponent,
                          ShaderComponent, MeshComponent>()
                      .entities()) {
        // std::print("render entity: \n");
        auto [shader, matrix, mesh] =
            a.data<ShaderComponent, TransformMatrixComponent, MeshComponent>(e);
        // if()
        _gl glUseProgram(shader->program_id);
        _gl glUniformMatrix4fv(
            _gl glGetUniformLocation(shader->program_id, "model"), 1,
            _gl GL_FALSE, glm::value_ptr(matrix->matrix));
        _gl glBindVertexArray(mesh->vao_id);
        if (mesh->ebo_id == 0)
            _gl glDrawArrays(_gl GL_TRIANGLES, mesh->draw_offset,
                             mesh->draw_count);
        else {
            _gl glDrawElements(_gl GL_TRIANGLES, mesh->draw_count,
                               _gl GL_UNSIGNED_INT,
                               (const void*)((size_t)mesh->draw_offset));
        }
    }
    // std::print("render mesh end\n");
}
void SceneWindow::render() {
    _gl glClear(_gl GL_COLOR_BUFFER_BIT);
    update_world_();
}

void SceneWindow::init_() {
    init_editors_();
    init_world_();
};
bool SceneWindow::event(AbsEvent* event) {
    auto e = dynamic_cast<Event*>(event);
    if (e->type() == EventType::WindowCloseRequested && !world_ready_stop_) {
        world_ready_stop_ = true;
        event_bus_.publish<WorldRequestExitEvent>();
        return true;
    }
    return GlfwImguiWindow::event(event);
};
SceneWindow::~SceneWindow() { std::println("SceneWindow destroy"); }
void SceneWindow::add_component_to_world_() {
    using namespace xc::xcal;
    ui_editor_cacher_->transform_component.state = TransformState::Dirty;
    world_.submit().create_entity(ui_editor_cacher_->transform_component,
                                  TransformMatrixComponent{});
}
void SceneWindow::init_editors_() {
    ui_editor_cacher_ = std::make_unique<UiEditorCacher>();
    add_editor([this](std::string& name) { name_ = name; }, "global", true,
               "name", name_);
    add_editor(
        [this](float r, float g, float b) {
            color_ = {r, g, b};
            _gl glClearColor(r, g, b, 1.0f);
            std::print("clear color: {} {} {}\n", r, g, b);
        },
        "clear color", true, "r", 0.3f, "g", 0.3f, "b", 0.3f);

    add_button([this]() { create_trangle_entity_(); }, "add component");
    add_editor(
        [this](float x, float y, float z) {
            auto& transform_component = ui_editor_cacher_->transform_component;
            transform_component.position = {x, y, z};
            std::print("transform position: {} {} {}\n", x, y, z);
        },
        "transform position", true, "x", 0.0f, "y", 0.0f, "z", 0.0f);
}
void SceneWindow::init_world_() {
    using namespace xc::xcal;
    world_.add_component<TransformComponent>()
        .add_component<TransformMatrixComponent>()
        .add_component<ShaderComponent>()
        .add_component<MeshComponent>();

    world_.add_resource<EventLoop>(loop())
        .add_resource<ecs::EventBus>(&event_bus_)
        .add_resource<ecs::ResourceTable>();
}
void SceneWindow::create_trangle_entity_() {
    using namespace xc::xcal;
    auto mesh =
        TrangleMesh(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(-0.5f, -0.5f, 0.0f),
                    glm::vec3(0.5f, -0.5f, 0.0f));
    auto shdaer = world_.resource<ecs::ResourceTable>()
                      .create_or_get<Shader, TrangleMesh>("./res/line.vs",
                                                          "./res/line.fs");

    auto shader_component = ShaderComponent{.program_id = shdaer->program};
    auto mesh_comp = mesh.mesh_component();
    auto& transform_component = ui_editor_cacher_->transform_component;
    transform_component.state = TransformState::Dirty;
    world_.submit().create_entity(transform_component, shader_component,
                                  mesh_comp, TransformMatrixComponent{});
    
                                };
void SceneWindow::update_world_() {
    world_.run_system<xc::xcal::update_transform_matrix>()
        .run_system<[](ecs::EventBus& bus, EventLoop& loop) {
            if (bus.exist<WorldeadyToExitEvent>()) {
                loop.publish(std::make_unique<Event>(
                    EventType::WindowCloseRequested, nullptr));
            }
        }>()
        .run_system<render_mesh>()
        .execute_commands();
};

// .run_system<[](ecs::EventBus& bus, ecs::ResourceTable& table) {
//     if (bus.exist<WorldeadyToExitEvent>()) return;
//     if (bus.exist<WorldRequestExitEvent>()) {
//         table.release_resource<TrangleMesh>();
//         table.release_resource<Shader, TrangleMesh>();
//         return;
//     }
//     if (!table.has_resource<TrangleMesh>()) {
//         table.create_or_get<TrangleMesh>(glm::vec3(0.0f, 0.5f, 0.0f),
//                                          glm::vec3(-0.5f, -0.5f, 0.0f),
//                                          glm::vec3(0.5f, -0.5f, 0.0f));
//     }
//     if (!table.has_resource<Shader, TrangleMesh>()) {
//         table.create_or_get<Shader, TrangleMesh>("./res/line.vs",
//                                                  "./res/line.fs");
//     }
//     auto trangle = table.get_resource<TrangleMesh>();
//     auto shader = table.get_resource<Shader, TrangleMesh>();
//     shader->use();
//     trangle->draw();
// }>()