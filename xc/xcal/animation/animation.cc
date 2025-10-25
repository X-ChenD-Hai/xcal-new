#include "./animation.hpp"

#include <ecs/world.hpp>

#include "./animation_manager.hpp"
#include "./translate.hpp"

void xc::xcal::animation::details::setup(ecs::World &world) {
    std::println("setup animation");
    world.add_resource<AnimationManager>();
    world.add_component<TranslateAnimationComponent>();
};
void xc::xcal::animation::details::run(ecs::World &world) {
    world.run_system<update_animation>().run_system<update_translate>();
}
void xc::xcal::animation::update_animation(AnimationManager &animation_manager,
                                           ecs::EventBus &event_bus) {
    auto time_line = animation_manager.current_time_line();
    if (time_line == nullptr) return;
    auto delta_time = time_line->update();
    if (delta_time <= 0.0f) return;
    std::println("delta_time: {}", delta_time);
    event_bus.publish<Update>(delta_time);
}
