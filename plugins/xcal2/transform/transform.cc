#include "transform.hpp"

#include "ecs/component_accessor.hpp"
#include "ecs/querier.hpp"
#include "ecs/world.hpp"


xcmath::mat4f xcal::transform::TransformComponent::transform_matrix() const {
    return xcmath::translate(
        xcmath::rotate(
            xcmath::rotate(
                xcmath::rotate(xcmath::scale(xcmath::mat4f::eye(), scale),
                               rotation.x(), {1.0f, 0.0f, 0.0f}),
                rotation.y(), {0.0f, 1.0f, 0.0f}),
            rotation.z(), {0.f, 0.0f, 1.0f}),
        position);
}
void update_transform_matrix(ecs::Querier q, ecs::ComponentAccessor a) {
    using namespace xcal::transform;
    for (auto e :
         q.query<TransformComponent, TransformMatrixComponent>().entities()) {
        auto t = a.data<TransformComponent>(e);
        if (t->state.has(TransformState::Dirty)) {
            auto m = a.data<TransformMatrixComponent>(e);
            m->matrix = t->transform_matrix();
            t->state.remove(TransformState::Dirty);
            std::println("update transform matrix {}", m->matrix);
        }
    }
}

void xcal::transform::TransformPlugin::install(ecs::World& world) {
    std::println("transform setup");
    world.regist_component<TransformComponent>()
        .regist_component<TransformMatrixComponent>();
}
void xcal::transform::TransformPlugin::run(ecs::World& world) {
    world.run_system<update_transform_matrix>();
}
