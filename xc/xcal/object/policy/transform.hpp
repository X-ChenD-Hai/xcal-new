#pragma once
#include <ecs/entity.hpp>
#include <ecs/world.hpp>
#include <xcal/transform/transform.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::object::policy {
struct Transform {
    static inline transform::TransformComponent* ensure_transform(
        ecs::World& w, ecs::Entity e) {
        return w
            .template get_or_attach_component<transform::TransformComponent>(e);
    }
    static inline void set_pos(ecs::World& w, ecs::Entity e,
                               const xcmath::vec3f& p) {
        auto* t = ensure_transform(w, e);
        t->position = p;
        t->state.add(transform::TransformState::Dirty);
    }
    static inline void set_rotation(ecs::World& w, ecs::Entity e,
                                    const xcmath::vec3f& r) {
        auto* t = ensure_transform(w, e);
        t->rotation = r;
        t->state.add(transform::TransformState::Dirty);
    }
    static inline void set_scale(ecs::World& w, ecs::Entity e,
                                 const xcmath::vec3f& s) {
        auto* t = ensure_transform(w, e);
        t->scale = s;
        t->state.add(transform::TransformState::Dirty);
    }
    static inline void set_transform(ecs::World& w, ecs::Entity e,
                                     const transform::TransformComponent& t) {
        auto c = ensure_transform(w, e);
        *c = t;
        c->state.add(transform::TransformState::Dirty);
    }
    static inline const transform::TransformComponent& transform(
        ecs::World& w, ecs::Entity e) {
        return *ensure_transform(w, e);
    }
};

}  // namespace xc::xcal::object::policy