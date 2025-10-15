#pragma once
#include <cstdint>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::event {

struct FrameResize {
    uint32_t width;
    uint32_t height;
};

struct CameraViewChanged {};
struct CameraProjectionChanged {};

}  // namespace xc::xcal::event