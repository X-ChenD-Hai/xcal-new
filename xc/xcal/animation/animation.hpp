#pragma once
#include <ecs/component_accessor.hpp>
#include <xc_assert.hpp>
#include <xcmath/xcmath.hpp>
namespace ecs {
class World;
}
namespace xc::xcal::animation::interpolation_function {
using interpolation_function = float (*)(float);
inline constexpr float linear(float t) noexcept { return t; }
};  // namespace xc::xcal::animation::interpolation_function

namespace xc::xcal::animation {
struct AnimationComponent {
    interpolation_function::interpolation_function interpolation_func;
    float current_duration;
    inline bool is_valid() const noexcept {
        return current_duration >= 0.0f && current_duration <= 1.0f;
    }
    inline float coefficient() const noexcept {
        XC_ASSERT(is_valid());
        return interpolation_func(current_duration);
    }
    inline bool update(float delta_time) noexcept {
        current_duration += delta_time;
        return is_valid();
    }
};

struct Update {
    float delta_time;
};
namespace details {
void setup(ecs::World &world);
void run(ecs::World &world);
}
}  // namespace xc::xcal::animation