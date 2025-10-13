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
#include <ecs/Application.hpp>
#include <print>
#include <xcmath/xcmath.hpp>

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

struct UniformBuffer {
    _gl GLuint ubo;

    glm::vec3 position{0.0f, 0.0f, -1.0f};
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    glm::vec3 up{0.f, 1.0f, 0.f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float fov{45.0f};
    float aspect{1.0f};
    float near{0.1f};
    float far{100.0f};

    UniformBuffer() {
        _gl glGenBuffers(1, &ubo);
        _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
        _gl glBufferData(_gl GL_UNIFORM_BUFFER, sizeof(glm::mat4),
                         glm::value_ptr(glm::mat4(1.0f)), _gl GL_DYNAMIC_DRAW);
    }
    void update() {
        auto pv = glm::perspective(glm::radians(fov), aspect, near, far) *
                  glm::lookAt(position, center, up);

        _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
        _gl glBufferSubData(_gl GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                            glm::value_ptr(pv));
        _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, 0);
    }
    void bind() {
        _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, ubo);
        _gl glBindBufferBase(_gl GL_UNIFORM_BUFFER, 1, ubo);
        // _gl glBindBuffer(_gl GL_UNIFORM_BUFFER, 0);
    }
};

void render_mesh(ecs::Querier q, ecs::ComponentAccessor a) {
    using namespace xc::xcal;

    a.each<TransformMatrixComponent, ShaderComponent, MeshComponent>(
        [](auto& t, auto& s, auto& m) {
            _gl glUseProgram(s.program_id);
            _gl glUniformMatrix4fv(
                _gl glGetUniformLocation(s.program_id, "model"), 1,
                _gl GL_FALSE, glm::value_ptr(t.matrix));
            _gl glBindVertexArray(m.vao_id);
            if (m.ebo_id == 0)
                _gl glDrawArrays(_gl GL_TRIANGLES, m.draw_offset, m.draw_count);
            else {
                _gl glDrawElements(_gl GL_TRIANGLES, m.draw_count,
                                   _gl GL_UNSIGNED_INT,
                                   (const void*)((size_t)m.draw_offset));
            }
        });
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

    world_.use_plugin<xc::xcal::Application>();
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
    world_.run_plugin<xc::xcal::Application>()
        .run_system<xc::xcal::update_transform_matrix>()
        .run_system<&SceneWindow::update_camera_>(this)
        .run_system<[](ecs::EventBus& bus, EventLoop& loop) {
            if (bus.exist<WorldeadyToExitEvent>()) {
                loop.publish(std::make_unique<Event>(
                    EventType::WindowCloseRequested, nullptr));
            }
        }>()
        .run_system<render_mesh>()
        .execute_commands();
};

bool SceneWindow::resize_event(WindowResizeEvent* e) {
    // std::print("resize event: {} {}\n", e->width(), e->height());
    event_bus_.publish<xc::xcal::event::FrameResize>(e->width(), e->height())
        .publish<xc::xcal::event::CameraProjectionChanged>(
            45.0f, (float)e->width() / (float)e->height(), 0.1f, 100.0f);

    return GlfwImguiWindow::resize_event(e);
}
void SceneWindow::update_camera_(ecs::EventBus& event_bus,
                                 ecs::ResourceTable& tab) {
    event_bus.each<xc::xcal::event::FrameResize>(
        [](auto& e) { _gl glViewport(0, 0, e.width, e.height); });
    event_bus.each<xc::xcal::event::CameraProjectionChanged>(
        [](xc::xcal::event::CameraProjectionChanged& e, auto& tab) {
            auto ubo = tab.template create_or_get<UniformBuffer>();
            ubo->fov = e.fov;
            ubo->aspect = e.aspect;
            ubo->near = e.near;
            ubo->far = e.far;
            ubo->update();
            ubo->bind();
        },
        tab);
}
