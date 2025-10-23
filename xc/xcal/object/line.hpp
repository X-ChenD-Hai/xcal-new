#pragma once
#include <xcmath/xcmath.hpp>

#include "./object.hpp"

namespace xc::xcal::object {
class Line : public Object {
    friend class xcal::Xcal;

   public:
    struct Config {
        xcmath::vec3f direction{};
    };

   private:
    Config config_;

   public:
    Line(xcmath::vec3f direction) : Object(), config_{direction} {}

    ~Line() override = default;
};
}  // namespace xc::xcal::object