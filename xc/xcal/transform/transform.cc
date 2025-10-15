#include "./transform.hpp"
#include <xcmath/utils/show.hpp>
xcmath::mat4f xc::xcal::transform::TransformComponent::transform_matrix()
    const {
    return xcmath::translate(
        xcmath::rotate(
            xcmath::rotate(
                xcmath::rotate(xcmath::scale(xcmath::mat4f::eye(), scale),
                               rotation.x(), {1.0f, 0.0f, 0.0f}),
                rotation.y(), {0.0f, 1.0f, 0.0f}),
            rotation.z(), {0.f, 0.0f, 1.0f}),
        position);
}
void xc::xcal::transform::update_transform_matrix(ecs::Querier q,
                                                  ecs::ComponentAccessor a) {
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
