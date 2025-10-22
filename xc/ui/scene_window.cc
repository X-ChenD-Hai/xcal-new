#include <ecs/event_bus.hpp>
#include <ecs/resource.hpp>
#include <xcal/camera/camera.hpp>
#include <xcal/event/events.hpp>
#include <xcal/render/backend/opengl/mesh.hpp>
#include <xcal/render/backend/opengl/mesh/axis.hpp>
#include <xcal/render/backend/opengl/mesh/trangle.hpp>
#include <xcal/render/backend/opengl/render.hpp>
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
#include <print>
#include <xcal/ecs/application.hpp>
#include <xcmath/xcmath.hpp>

#include "./scene_window.hpp"

struct UiEditorCacher {
    xc::xcal::transform::TransformComponent transform_component;
};

SceneWindow::SceneWindow(const std::string& name, int width, int height,
                         int fps)
    : GlfwImguiWindow(name, width, height, fps) {
    init_();
}
SceneWindow::SceneWindow() : GlfwImguiWindow() { init_(); }

void SceneWindow::render() {
    _gl glClear(_gl GL_COLOR_BUFFER_BIT | _gl GL_DEPTH_BUFFER_BIT);
    update_world_();
}

void SceneWindow::init_() {
    init_editors_();
    _gl glEnable(_gl GL_DEPTH_TEST);
    _gl glDepthFunc(_gl GL_LESS);
    _gl glEnable(_gl GL_BLEND);
    _gl glBlendFunc(_gl GL_SRC_ALPHA, _gl GL_ONE_MINUS_SRC_ALPHA);
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

    add_button([this]() { create_entity_(); }, "add component");
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
        .use_plugin<Application>()
        .use_plugin<Render>();
}
void SceneWindow::create_entity_() {
    using namespace xc::xcal;
    using namespace xc::xcal::render::opengl;
    world_.plugin<Render>().add_mesh(
        Trangle({0.0f, 0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {-0.5f, -0.5f, 0.0f}),
        ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(Line({2.0f, 0.f, 0.0f}),
                                     ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(Line({.0f, 2.f, 0.0f}),
                                     ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(Line({.0f, 0.f, 2.0f}),
                                     ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(
        ParametricCurve{[](auto t) { return xcmath::vec3f{t, t * t, 0.f}; },
                        0.0f, 1.0f, 1},
        ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(
        ParametricCurve{[](auto t) { return xcmath::vec3f{t, t * t, 0.f}; },
                        0.0f, 1.0f, 100},
        ui_editor_cacher_->transform_component);
    world_.plugin<Render>().add_mesh(
        ParametricSurface(
            [](auto u, auto v) {  // u=θ∈[0,2π], v=φ∈[0,π]
                float r = .3f;    // 球半径
                float sin_v = std::sin(v);
                return xcmath::vec3f{r * std::cos(u) * sin_v,
                                     r * std::sin(u) * sin_v, r * std::cos(v)};
            },
            0.0f, xcmath::PI * 2, 20,  // θ 方向
            0.0f, xcmath::PI, 20       // φ 方向
            ),
        ui_editor_cacher_->transform_component);
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
        // std::println("key event {} {}", e->action(), (int)e->key());
        if (e->key() == Key::W) {
            // std::print("key event: W\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::FORWARD, speed);
        } else if (e->key() == Key::S) {
            // std::print("key event: S\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::BACKWARD, speed);
        } else if (e->key() == Key::A) {
            // std::print("key event: A\n");
            world_.resource<FpsCameraControler>().move(
                FpsCameraControler::Direction::LEFT, speed);
        } else if (e->key() == Key::D) {
            // std::print("key event: D\n");
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
    static constexpr float zoom_speed = .05f;
    world_.resource<xc::xcal::camera::FpsCameraControler>().zoom(
        -e->y_offset() * zoom_speed);
    return GlfwImguiWindow::wheel_event(e);
}
bool SceneWindow::mouse_move_event(MouseMoveEvent* e) {
    if (moving) {
        auto x_offset = e->x_pos() - last_mouse_pos_.x_pos;
        auto y_offset = e->y_pos() - last_mouse_pos_.y_pos;
        world_.resource<xc::xcal::camera::FpsCameraControler>().rotate(
            x_offset, -y_offset);
        // std::println("x {}, y {}", x_offset, y_offset);
        last_mouse_pos_.x_pos = e->x_pos();
        last_mouse_pos_.y_pos = e->y_pos();
    }
    return GlfwImguiWindow::mouse_move_event(e);
}
bool SceneWindow::mouse_button_event(MouseButtonEvent* e) {
    if (e->button() == MouseButtons::Left) {
        if (e->action() == KeyAction::Press) {
            set_cursor_mode(CursorMode::Disabled);
            moving = true;
            last_mouse_pos_.x_pos = e->x_pos();
            last_mouse_pos_.y_pos = e->y_pos();
        } else if (e->action() == KeyAction::Release) {
            set_cursor_mode(CursorMode::Normal);
            moving = false;
        }
    }
    return GlfwImguiWindow::mouse_button_event(e);
}
