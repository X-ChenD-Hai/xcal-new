#pragma once
#include <cstdint>
#include <xcmath/xcmath.hpp>

namespace xcal::event {
struct FrameResize {
    uint32_t width;
    uint32_t height;
};
struct CameraChanged {
    xcmath::vec3f position;
    xcmath::vec3f direction;
    xcmath::vec3f up;
};
struct CameraProjectionChanged {
    float fov;
    float aspect;
    float near;
    float far;
};

struct ViewMatrixUpdate {
    xcmath::mat4f view_matrix;
};
struct ProjectionMatrixUpdate {
    xcmath::mat4f projection_matrix;
};

}  // namespace xcal::event