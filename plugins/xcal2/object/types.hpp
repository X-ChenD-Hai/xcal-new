#pragma once

#include <cstdint>
#include <functional>

#include "xcmath/mobject/declaration.hpp"
#include "xcmath/mobject/function.hpp"
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
    double operator()(double x) const { return function(x); }
    FunctionCurve2d derivative() const {
        auto fn = function;
        auto dx = (max_x - min_x) / num_samples;
        return FunctionCurve2d(
            [fn, dx](double x) { return xcmath::derivative(dx, fn, x); }, min_x,
            max_x, num_samples);
    }
};

struct ParametricCurve {
    std::function<xcmath::vec3f(float)> function;
    float min_t{0.0};
    float max_t{1.0};
    uint32_t num_samples{100};
    ParametricCurve(std::function<xcmath::vec3f(double)> function,
                    float min_t = 0.0, float max_t = 1.0,
                    uint32_t num_samples = 100)
        : function(function),
          min_t(min_t),
          max_t(max_t),
          num_samples(num_samples) {}
    xcmath::vec3f operator()(float t) const { return function(t); }
    ParametricCurve derivative() const {
        auto fn = function;
        auto dt = (max_t - min_t) / num_samples;
        return ParametricCurve(
            [fn, dt](float t) { return xcmath::derivative(dt, fn, t); }, min_t,
            max_t, num_samples);
    }
};
struct ParametricSurface {
    std::function<xcmath::vec3f(float, float)> function;
    float min_u{0.0};
    float max_u{1.0};
    uint32_t num_samples_u{100};
    float min_v{0.0};
    float max_v{1.0};
    uint32_t num_samples_v{100};
    ParametricSurface(std::function<xcmath::vec3f(float, float)> function,
                      float min_u = 0.0, float max_u = 1.0,
                      uint32_t num_samples_u = 100, float min_v = 0.0,
                      float max_v = 1.0, uint32_t num_samples_v = 100)
        : function(function),
          min_u(min_u),
          max_u(max_u),
          num_samples_u(num_samples_u),
          min_v(min_v),
          max_v(max_v),
          num_samples_v(num_samples_v) {}
    xcmath::vec3f operator()(float u, float v) const { return function(u, v); }

};

struct QuadraticBezierCurve2d {
    xcmath::vec3f p0{0.0f, 0.0f};
    xcmath::vec3f p1{0.0f, 0.0f};
    xcmath::vec3f p2{0.0f, 0.0f};
};
}  // namespace xcal::object