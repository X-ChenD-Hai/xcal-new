#include "./camera.hpp"

#include <ecs/event_bus.hpp>
#include <ecs/resource.hpp>
#include <ecs/resource_table.hpp>
#include <ecs/world.hpp>
#include <print>

#include "../event/events.hpp"

void xc::xcal::camera::update_camera(ecs::ResourceManager& mgr,
                                     ecs::EventBus& event_bus) {
    event_bus.each<xc::xcal::event::FrameResize>(
        [](auto& e, auto& mgr, auto& bus) {
            std::println("resource size {}", mgr.size());
            std::println("resource id {}", mgr.template id<ProjectionConfig>());
            mgr.template get<ProjectionConfig>().aspect =
                (float)e.width / (float)e.height;
            bus.template publish<event::CameraProjectionChanged>();
        },
        mgr, event_bus);
}
xcmath::mat4f xc::xcal::camera::ProjectionConfig::as_mat4() const {
    auto P = xcmath::mat4f::eye();

    const float fov_rad = fov * xcmath::PI / 180.0f;
    const float tan_half = std::tan(fov_rad * 0.5f);
    const float n = near;
    const float f = far;
    const float a = aspect;

    // 行主序
    P[0][0] = 1.0f / (a * tan_half);
    P[1][1] = 1.0f / tan_half;
    P[2][2] = -(f + n) / (f - n);
    P[2][3] = -2.0f * f * n / (f - n);
    P[3][2] = -1.0f;
    return P;
}
xcmath::mat4f xc::xcal::camera::ViewConfig::as_mat4() const {
    auto V = xcmath::mat<float_t, 4, 4>{0.0f};

    using vec3 = xcmath::vec<float_t, 3>;
    const vec3 eye = position;
    const vec3 center = position + direction.normalize();

    const vec3 f = (center - eye).normalize();
    const vec3 r = f.cross(up).normalize();
    const vec3 u = r.cross(f);

    // 行主序 look-at
    V[0][0] = r.x();
    V[0][1] = r.y();
    V[0][2] = r.z();
    V[0][3] = -r.dot(eye);
    V[1][0] = u.x();
    V[1][1] = u.y();
    V[1][2] = u.z();
    V[1][3] = -u.dot(eye);
    V[2][0] = -f.x();
    V[2][1] = -f.y();
    V[2][2] = -f.z();
    V[2][3] = f.dot(eye);
    V[3][3] = 1.0f;
    return V;
}
void xc::xcal::camera::FpsCameraControler::move(Direction direction,
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
    event_bus_->publish<event::CameraViewChanged>();
}
void xc::xcal::camera::FpsCameraControler::rotate(float dyaw, float dpitch) {
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
    event_bus_->publish<event::CameraViewChanged>();
}
void xc::xcal::camera::FpsCameraControler::zoom(float dzoom) {
    zoon_ += dzoom;
    projection_->fov = std::clamp(fov_ * zoon_, 1.0f, 179.0f);
    zoon_ = projection_->fov / fov_;
    event_bus_->publish<event::CameraProjectionChanged>();
}
void xc::xcal::camera::details::setup(ecs::World& world) {
    std::println("camera setup");
    XC_ASSERT(world.resource_manager().has<ecs::EventBus>());
    world.add_resource<ViewConfig>()
        .add_resource<ProjectionConfig>()
        .add_resource<FpsCameraControler>(&world.resource<ecs::EventBus>(),
                                          &world.resource<ViewConfig>(),
                                          &world.resource<ProjectionConfig>());
}
void xc::xcal::camera::details::run(ecs::World& world) {
    world.run_system<update_camera>();
}
