#pragma once
#include "./camera.hpp"
namespace xcal::camera {
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
    ecs::EventBus* event_bus_{nullptr};
    ViewConfig* camera_{nullptr};
    ProjectionConfig* projection_{nullptr};

    float yaw_{270.0f};
    float pitch_{0.0f};

    float zoon_{1.0f};
    float fov_{45.0f};

   public:
    explicit FpsCameraControler(ecs::EventBus* event_bus, ViewConfig* camera,
                                ProjectionConfig* projection = nullptr)
        : event_bus_(event_bus), camera_(camera), projection_(projection) {
        if (projection_) fov_ = projection_->fov;
    }

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
    void zoom(float dzoom);
};
}