#include "./translate.hpp"

#include <ecs/world.hpp>
#include <print>

#include "../transform/transform.hpp"


void xc::xcal::animation::update_translate(ecs::ComponentAccessor accessor,
                                           const ecs::EventBus &event_bus) {
    event_bus.each<Update>(
        [](auto &event, auto &accessor) {
            accessor.template each<transform::TransformComponent,
                                   TranslateAnimationComponent>(
                [](auto &transform, auto &translate, auto &update) {
                    auto delta_time = update.delta_time;
                    std::println("delta_time: {}", delta_time);
                },
                event);
        },
        accessor);
}
