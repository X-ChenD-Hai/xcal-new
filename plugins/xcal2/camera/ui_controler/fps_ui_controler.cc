#include "fps_ui_controler.hpp"

#include <ui_protocol/types.hpp>
#include <xc/ecs/event_bus.hpp>
#include <xcal2/camera/fps_camera_controler.hpp>

void xcal::camera::ui_controler::FPSUIControler::run(
    ecs::EventBus& bus, xcal::camera::FpsCameraControler& camera_controler) {
    using namespace ui_protocol;
    using namespace xcal::camera;
    using Direction = xcal::camera::FpsCameraControler::Direction;
    static bool left_pressed{false};
    bus.each([&](KeyEvent& e) {
        if (e.action == KeyAction::Press || e.action == KeyAction::Repeat) {
            if (e.key == Key::W || e.key == Key::UP) {
                camera_controler.move(Direction::FORWARD, dz_);
            } else if (e.key == Key::S || e.key == Key::DOWN) {
                camera_controler.move(Direction::BACKWARD, dz_);
            } else if (e.key == Key::A || e.key == Key::LEFT) {
                camera_controler.move(Direction::LEFT, dx_);
            } else if (e.key == Key::D || e.key == Key::RIGHT) {
                camera_controler.move(Direction::RIGHT, dx_);
            } else if (e.key == Key::E) {
                camera_controler.move(Direction::UP, dy_);
            } else if (e.key == Key::Q) {
                camera_controler.move(Direction::DOWN, dy_);
            }
        }
    });
    bus.each([&](WheelEvent& e) { camera_controler.zoom(-e.dy * dzoom_); });
    bus.each([&](MouseButtonEvent& e) {
        if (e.action == KeyAction::Press) {
            if (e.button == MouseButton::Left) {
                left_pressed = true;
                bus.publish<SetInputModeEvent>(InputMode::CursorDisabled);
            }
        } else if (e.action == KeyAction::Release) {
            if (e.button == MouseButton::Left) {
                left_pressed = false;
                bus.publish<SetInputModeEvent>(InputMode::CursorNormal);
            }
        }
    });
    bus.each([&](MouseMoveEvent& e) {
        static constexpr float spead = 1;
        if (left_pressed) {
            camera_controler.rotate(e.dx * dyaw_, -e.dy * dpitch_);
        }
    });
    bus.each([&](WindowResizeEvent& e) {
        camera_controler.resize(e.width, e.height);
    });
}
