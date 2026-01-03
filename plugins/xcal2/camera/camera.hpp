#pragma once
#include <xcmath/xcmath.hpp>
namespace ecs {
class World;
class EventBus;
class ResourceTable;
class ResourceManager;
}  // namespace ecs

namespace xcal::camera {

struct ViewConfig {
    xcmath::vec3f position{0.0f, 0.0f, 1.f};
    xcmath::vec3f direction{0.0f, 0.0f, -1.0f};
    xcmath::vec3f up{0.0f, 1.0f, 0.0f};

    xcmath::mat4f as_mat4() const;
};
struct ProjectionConfig {
    float fov{45.0f};
    float aspect{1.0f};
    float near{0.1f};
    float far{100.0f};

    xcmath::mat4f as_mat4() const;
};



class CameroPlugin {
    friend class ecs::World;
    protected:
     static void install(ecs::World& world);
     static void run(ecs::World& world);
};

};  // namespace xcal::camera