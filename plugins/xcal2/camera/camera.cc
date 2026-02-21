#include "./camera.hpp"

#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/resource.hpp>
#include <xc/ecs/resource_table.hpp>
#include <xc/ecs/world.hpp>
#include <print>

#include "../events/events.hpp"

void update_camera(ecs::ResourceManager& mgr, ecs::EventBus& event_bus) {
    using namespace xcal::events;
    using namespace xcal::camera;
    event_bus.each(
        [](xcal::events::FrameResize& e, ecs::ResourceManager& mgr, ecs::EventBus& bus) {
            std::println("resource size {}", mgr.size());
            std::println("resource id {}", mgr.template id<ProjectionConfig>());
            mgr.template get<ProjectionConfig>().aspect =
                (float)e.width / (float)e.height;
            bus.template publish<CameraProjectionChanged>();
        },
        mgr, event_bus);
}
xcmath::mat4f xcal::camera::ProjectionConfig::as_mat4() const {
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
xcmath::mat4f xcal::camera::ViewConfig::as_mat4() const {
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

void xcal::camera::Camero::install(ecs::World& world) {
    std::println("camera setup");

    XC_ASSERT(world.resource_manager().has<ecs::EventBus>());
    world.add_resource<ViewConfig>().add_resource<ProjectionConfig>();
    world.resource<ecs::EventBus>().publish<events::CameraProjectionChanged>();
    world.resource<ecs::EventBus>().publish<events::CameraViewChanged>();
}
void xcal::camera::Camero::run(ecs::World& world) {
    world.run_system<update_camera>();
}
