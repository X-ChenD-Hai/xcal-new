#include "./animation.hpp"

#include <xc/ecs/world.hpp>

#include "./translate.hpp"

void xc::xcal::animation::details::setup(ecs::World& world) {
    std::println("setup animation");
    world.regist_component<TranslateAnimationComponent>();
};
void xc::xcal::animation::details::run(ecs::World& world) {
    world.run_system<update_translate>();
}
