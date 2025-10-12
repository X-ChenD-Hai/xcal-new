#define GLM_ENABLE_EXPERIMENTAL
#include "./systems.hpp"

#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
void xc::xcal::TransformComponent::apply_transform(glm::mat4 &matrix) const {
    matrix = glm::translate(matrix, position) * glm::scale(matrix, scale) *
             glm::rotate(matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
             glm::rotate(matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
             glm::rotate(matrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
}
void xc::xcal::update_transform_matrix(ecs::Querier q,
                                       ecs::ComponentAccessor a) {
    for (auto e :
         q.query<TransformComponent, TransformMatrixComponent>().entities()) {
        auto t = a.data<TransformComponent>(e);
        if (t->state.has(TransformState::Dirty)) {
            auto m = a.data<TransformMatrixComponent>(e);
            m->matrix = glm::mat4(1.0f);
            t->apply_transform(m->matrix);
            t->state.remove(TransformState::Dirty);
            std::println("update transform matrix {}",
                         glm::to_string(m->matrix));
        }
    }
}

