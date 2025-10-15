#pragma once
#include <algorithm>
#include <xcmath/xcmath.hpp>
namespace ecs {
class World;
class EventBus;
class ResourceTable;
class ResourceManager;
}  // namespace ecs

namespace xc::xcal::camera {

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

class FpsCameraControler {
   public:
    enum class Direction : uint8_t {
        FORWARD = 1,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
    };

   private:
    ViewConfig* camera_{nullptr};
    ecs::EventBus* event_bus_{nullptr};

    float yaw_{270.0f};
    float pitch_{0.0f};

   public:
    explicit FpsCameraControler(ViewConfig* camera, ecs::EventBus* event_bus)
        : camera_(camera), event_bus_(event_bus) {}

   public:
    void set_camera(ViewConfig* camera) {
        camera_ = camera;
        yaw_ = 90.0f;
        pitch_ = 0.0f;
    }
    xcmath::vec3<float> forward() const {
        return camera_->direction.normalize();
    }
    xcmath::vec3<float> right() const {
        return forward().cross(camera_->up).normalize();
    }
    xcmath::vec3<float> up() const {
        return right().cross(forward()).normalize();
    }

    void move(Direction direction, float distance);

    void rotate(float dyaw, float dpitch);
};
void update_camera(ecs::ResourceManager& mgr, ecs::EventBus& event_bus);
};  // namespace xc::xcal::camera