#include "./transform.hpp"

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
                                                  ecs::ComponentAccessor a) {}
