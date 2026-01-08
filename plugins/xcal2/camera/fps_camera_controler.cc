#include "fps_camera_controler.hpp"

#include "../events/events.hpp"
#include "ecs/event_bus.hpp"

void xcal::camera::FpsCameraControler::move(Direction direction,
                                            float distance) {
    if (!camera_ || distance == 0.0f) return;
    xcmath::vec3<float> shift{0.0f, 0.0f, 0.0f};
    switch (direction) {
        case Direction::FORWARD:
            shift = +forward() * distance;
            break;
        case Direction::BACKWARD:
            shift = -forward() * distance;
            break;
        case Direction::LEFT:
            shift = -right() * distance;
            break;
        case Direction::RIGHT:
            shift = +right() * distance;
            break;
        case Direction::UP:
            shift = +up() * distance;
            break;
        case Direction::DOWN:
            shift = -up() * distance;
            break;
        default:
            return;
            break;
    }
    camera_->position += shift;
    event_bus_->publish<events::CameraViewChanged>();
}
void xcal::camera::FpsCameraControler::rotate(float dyaw, float dpitch) {
    if (!camera_ || (dyaw == 0.0f && dpitch == 0.0f)) return;
    yaw_ += dyaw;
    pitch_ += dpitch;
    yaw_ = std::fmod(yaw_, 360.0f);
    if (yaw_ < 0.0f) yaw_ += 360.0f;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    auto yaw = xcmath::radians(yaw_);
    auto pitch = xcmath::radians(pitch_);
    xcmath::vec3<float> forward{
        (float)(std::cos(yaw) * std::cos(pitch)),
        (float)(std::sin(pitch)),
        (float)(std::sin(yaw) * std::cos(pitch)),
    };
    camera_->direction = forward.normalize();
    event_bus_->publish<events::CameraViewChanged>();
}
void xcal::camera::FpsCameraControler::zoom(float dzoom) {
    zoon_ += dzoom;
    projection_->fov = std::clamp(fov_ * zoon_, 1.0f, 179.0f);
    zoon_ = projection_->fov / fov_;
    event_bus_->publish<events::CameraProjectionChanged>();
}
void xcal::camera::FpsCameraControler::resize(int width, int height) {
    if (!projection_) return;
    projection_->aspect = (float)width / (float)height;
    event_bus_->publish<events::CameraProjectionChanged>();
}
