#pragma once

#include <cstdint>
#include <functional>

#include "xcmath/xcmath.hpp"

namespace xcal::object {
struct Path2dStyle {
    xcmath::vec3f stroke_color{1.0f, 1.0f, 1.0f};
    float stroke_width{1.0f};
    xcmath::vec3f fill_color{0.0f, 0.0f, 0.0f};
    bool is_filled{false};
};

struct FunctionCurve2d {
    std::function<double(double)> function;
    double min_x{0.0};
    double max_x{1.0};
    uint32_t num_samples{100};
    FunctionCurve2d(std::function<double(double)> function, double min_x = 0.0,
                    double max_x = 1.0, uint32_t num_samples = 100)
        : function(function),
          min_x(min_x),
          max_x(max_x),
          num_samples(num_samples) {}
};
}  // namespace xcal::object