#pragma once
#include <xcal/transform/transformable.hpp>
#include <xcmath/xcmath.hpp>

#include "./object.hpp"
#include "./policy/transform.hpp"

namespace xc::xcal::object {
class Line : public Object,
             public transform::Transformable<Line, policy::Transform> {
    friend class xcal::Xcal;

   public:
    struct Config {
        xcmath::vec3f direction{};
    };

   private:
    Config config_;

   public:
    Line(xcmath::vec3f direction) : Transformable(), config_{direction} {}

    ~Line() override = default;
};
}  // namespace xc::xcal::object