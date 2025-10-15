
#include <ecs/CommandSubmit.hpp>
#include <ecs/EventBus.hpp>
#include <ecs/Querier.hpp>
#include <ecs/Resource.hpp>
#include <ecs/ResourceTable.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <render/backend/opengl/Mesh.hpp>
#include <xcal/camera/Camera.hpp>
#include <xcal/event/events.hpp>
#include <xcal/render/backend/opengl/Render.hpp>
#include <xcal/render/backend/opengl/Shader.hpp>
#include <xcal/transform/transform.hpp>

#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

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

struct UiEditorCacher {
    xc::xcal::transform::TransformComponent transform_component;
};

SceneWindow::SceneWindow(const std::string& name, int width, int height,
                         int fps)
    : GlfwImguiWindow(name, width, height, fps) {
    init_();
}
SceneWindow::SceneWindow() : GlfwImguiWindow() { init_(); }

struct TrangleMesh {
    _gl GLuint vao, vbo, shader;
    TrangleMesh(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
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
    xc::xcal::render::opengl::MeshComponent mesh_component() {
        return {
            .vao_id = vao,
            .vbo_id = vbo,
            .ebo_id = 0,
            .draw_count = 3,
            .draw_offset = 0,
        };
    }
};
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
    using namespace xc::xcal::render::opengl;
    world_.add_resource<EventLoop>(loop())
        .add_resource<ecs::EventBus>(&event_bus_)
        .add_resource<ecs::ResourceTable>();
    world_.use_plugin<Application>().use_plugin<Render>();
}
void SceneWindow::create_trangle_entity_() {
    using namespace xc::xcal;
    using namespace xc::xcal::render::opengl;
    auto mesh =
        TrangleMesh(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(-0.5f, -0.5f, 0.0f),
                    glm::vec3(0.5f, -0.5f, 0.0f));
    auto shdaer = world_.resource<ecs::ResourceTable>()
                      .create_or_get<Shader, TrangleMesh>("./res/line.vs",
                                                          "./res/line.fs");

    auto shader_component = ShaderComponent{.program_id = shdaer->program};
    auto mesh_comp = mesh.mesh_component();
    auto& transform_component = ui_editor_cacher_->transform_component;
    transform_component.state = xc::xcal::transform::TransformState::Dirty;
    world_.submit().create_entity(
        transform_component, shader_component, mesh_comp,
        xc::xcal::transform::TransformMatrixComponent{});
};
void SceneWindow::update_world_() {
    world_.run_plugin<xc::xcal::Application>()
        .run_plugin<xc::xcal::render::opengl::Render>()
        .run_system<[](ecs::EventBus& bus, EventLoop& loop) {
            if (bus.exist<WorldeadyToExitEvent>()) {
                loop.publish(std::make_unique<Event>(
                    EventType::WindowCloseRequested, nullptr));
            }
        }>()
        .execute_commands()
        .resource<ecs::EventBus>()
        .clear_all();
};

bool SceneWindow::resize_event(WindowResizeEvent* e) {
    event_bus_.publish<xc::xcal::event::FrameResize>(e->width(), e->height());
    std::print("resize event: {} {}\n", e->width(), e->height());
    return GlfwImguiWindow::resize_event(e);
}
bool SceneWindow::key_event(KeyEvent* e) {
    using namespace xc::xcal::camera;
    static constexpr float speed = .01f;
    if (e->action() == KeyActions::Press || e->action() == KeyActions::Repeat) {
        std::println("key event {} {}", e->action(), (int)e->key());
        if (e->key() == Key::W) {
            std::print("key event: W\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::FORWARD, speed);
        } else if (e->key() == Key::S) {
            std::print("key event: S\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::BACKWARD, speed);
        } else if (e->key() == Key::A) {
            std::print("key event: A\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::LEFT, speed);
        } else if (e->key() == Key::D) {
            std::print("key event: D\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::RIGHT, speed);
        } else if (e->key() == Key::Q) {
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::DOWN, speed);
        } else if (e->key() == Key::E) {
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::UP, speed);
        }
    }

    return GlfwImguiWindow::key_event(e);
}
bool SceneWindow::wheel_event(WheelEvent* e) {
    return GlfwImguiWindow::wheel_event(e);
}
bool SceneWindow::mouse_move_event(MouseMoveEvent* e) {
    if (moving) {
        auto x_offset = e->x_pos() - last_mouse_pos_.x_pos;
        auto y_offset = e->y_pos() - last_mouse_pos_.y_pos;
        world_.resource<xc::xcal::camera::FpsCameraControler>().rotate(
            x_offset, -y_offset);
        std::println("x {}, y {}", x_offset, y_offset);
        last_mouse_pos_.x_pos = e->x_pos();
        last_mouse_pos_.y_pos = e->y_pos();
    }
    return GlfwImguiWindow::mouse_move_event(e);
}
bool SceneWindow::mouse_button_event(MouseButtonEvent* e) {
    if (e->action() == KeyAction::Press) {
        set_cursor_mode(CursorMode::Disabled);
        moving = true;
        last_mouse_pos_.x_pos = e->x_pos();
        last_mouse_pos_.y_pos = e->y_pos();
    } else if (e->action() == KeyAction::Release) {
        set_cursor_mode(CursorMode::Normal);
        moving = false;
    }
    return GlfwImguiWindow::mouse_button_event(e);
}
