#pragma once
#include <cstdint>
#include <ecs/entity.hpp>

namespace xcal::events {

struct FrameResize {
    uint32_t width;
    uint32_t height;
};

struct CameraViewChanged {};
struct CameraProjectionChanged {};

}  // namespace xcal::events