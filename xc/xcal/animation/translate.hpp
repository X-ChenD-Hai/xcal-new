#pragma once
#include <ecs/component_accessor.hpp>
#include <ecs/event_bus.hpp>
#include <xc_assert.hpp>
#include <xcmath/xcmath.hpp>

#include "./animation.hpp"

namespace xc::xcal::animation {

struct TranslateAnimationComponent {
    AnimationComponent animation;
    xcmath::vec3f start_pos;
    xcmath::vec3f end_pos;

    inline constexpr xcmath::vec3f current_pos() const noexcept {
        XC_ASSERT(animation.is_valid());
        return start_pos + (end_pos - start_pos) * animation.coefficient();
    }
};

void update_translate(ecs::ComponentAccessor accessor,
                      ecs::EventBus& event_bus);

};  // namespace xc::xcal::animation