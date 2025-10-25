#pragma once
#include <ecs/component_accessor.hpp>
#include <xc_assert.hpp>
#include <xcmath/xcmath.hpp>

#include "ecs/entity.hpp"
#include "xcal/object/object.hpp"

namespace ecs {
class World;
}
namespace xc::xcal::animation::interpolation_function {
using interpolation_function = float (*)(float);
inline constexpr float linear(float t) noexcept { return t; }
};  // namespace xc::xcal::animation::interpolation_function

namespace xc::xcal::animation {
class AnimationManager;
struct AnimationComponent {
    interpolation_function::interpolation_function interpolation_func;
    float current_duration;
    float duration;
    inline bool is_valid() const noexcept {
        return current_duration >= 0.0f && current_duration <= duration;
    }
    inline float coefficient() const noexcept {
        XC_ASSERT(is_valid());
        return interpolation_func(current_duration / duration);
    }
    inline bool update(float delta_time) noexcept {
        current_duration += delta_time;
        return is_valid();
    }
};

class Animation : public object::Object {
    AnimationComponent animation_component_;

   public:
    using Object::Object;
    Animation(const Object &object, float start_time, float end_time,
              interpolation_function::interpolation_function interpolation_func)
        : Object(object),
          animation_component_{
              .interpolation_func = interpolation_func,
              .current_duration = 0.0f,
              .duration = end_time - start_time,
          } {}
    AnimationComponent &animation_component() noexcept {
        return animation_component_;
    }
    virtual ~Animation() = default;
};

struct Update {
    float delta_time;
};

void update_animation(AnimationManager &animation_manager,
                      ecs::EventBus &event_bus);

namespace details {
void setup(ecs::World &world);
void run(ecs::World &world);
}  // namespace details
}  // namespace xc::xcal::animation