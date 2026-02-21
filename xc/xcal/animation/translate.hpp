#pragma once
#include <xc/ecs/component_accessor.hpp>
#include <xc/ecs/event_bus.hpp>
#include <xc/common/xc_assert.hpp>
#include <xcmath/xcmath.hpp>

#include "./animation.hpp"

namespace xc::xcal::animation {

struct TranslateAnimationComponent : public AnimationComponent {
    xcmath::vec3f start_pos;
    xcmath::vec3f end_pos;

    inline constexpr xcmath::vec3f current_pos() const noexcept {
        XC_ASSERT(is_valid());
        return start_pos + (end_pos - start_pos) * coefficient();
    }
};

void update_translate(ecs::ComponentAccessor accessor,
                      const ecs::EventBus& event_bus);

};  // namespace xc::xcal::animation