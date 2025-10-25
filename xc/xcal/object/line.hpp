#pragma once
#include <concepts>
#include <xcal/animation/animation.hpp>
#include <xcal/transform/transformable.hpp>
#include <xcmath/xcmath.hpp>

#include "./object.hpp"
#include "./policy/transform.hpp"
#include "xcal/animation/transform.hpp"

template <typename Derived>
class Animationable {
   public:
    decltype(auto) animation(
        float start_time, float end_time,
        xc::xcal::animation::interpolation_function::interpolation_function
            interpolation_func =
                xc::xcal::animation::interpolation_function::linear) {
        if constexpr (std::derived_from<
                          Derived,
                          xc::xcal::transform::Transformable<
                              Derived, xc::xcal::object::policy::Transform>>) {
            return xc::xcal::animation::Transform(start_time, end_time,
                                                  interpolation_func,
                                                  *static_cast<Derived*>(this));
        }
    }

    ~Animationable() = default;
};

namespace xc::xcal::object {
class Line : public Object,
             public transform::Transformable<Line, policy::Transform>,
             public Animationable<Line> {
    friend class xcal::Xcal;

   public:
    struct Config {
        xcmath::vec3f direction{};
    };

   private:
    Config config_;

   public:
    Line(xcmath::vec3f direction) : Transformable(), config_{direction} {}

    ~Line() = default;
};
}  // namespace xc::xcal::object