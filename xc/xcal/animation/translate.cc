#include "./translate.hpp"

#include <ecs/world.hpp>
#include <print>

#include "../transform/transform.hpp"

void xc::xcal::animation::update_translate(ecs::ComponentAccessor accessor,
                                           ecs::EventBus &event_bus) {
    event_bus.each(
        [](Update &event, ecs::ComponentAccessor &accessor) {
            accessor.each<transform::TransformComponent,
                          TranslateAnimationComponent>(
                [](transform::TransformComponent &transform,
                   TranslateAnimationComponent &translate, auto &update) {
                    if (!translate.animation.is_valid()) return;
                    auto delta_time = update.delta_time;
                    if (translate.animation.update(delta_time)) {
                        transform.position = translate.current_pos();
                        transform.state |= transform::TransformState::Dirty;
                        std::println("delta_time: {} ,pos {}", delta_time,
                                     transform.position);
                    }
                },
                event);
        },
        accessor);
}
