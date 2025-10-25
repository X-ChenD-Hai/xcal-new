#pragma once
#include <ecs/entity.hpp>
#include <ecs/world.hpp>
#include <xcal/transform/transform.hpp>
#include <xcmath/xcmath.hpp>

#include "xcal/object/object.hpp"

namespace xc::xcal::object::policy {
struct Transform {
    static inline void set_pos(Object& o, const xcmath::vec3f& p) {
        auto& t = o.component<transform::TransformComponent>();
        t.position = p;
        t.state.add(transform::TransformState::Dirty);
    }
    static inline void set_rotation(Object& o, const xcmath::vec3f& r) {
        auto& t = o.component<transform::TransformComponent>();
        t.rotation = r;
        t.state.add(transform::TransformState::Dirty);
    }
    static inline void set_scale(Object& o, const xcmath::vec3f& s) {
        auto& t = o.component<transform::TransformComponent>();
        t.scale = s;
        t.state.add(transform::TransformState::Dirty);
    }
    static inline void set_transform(Object& o,
                                     const transform::TransformComponent& t) {
        auto& c = o.component<transform::TransformComponent>();
        c = t;
        c.state.add(transform::TransformState::Dirty);
    }
    static inline const transform::TransformComponent& transform(
        const Object& o) {
        static transform::TransformComponent default_transform;
        if (!o.has_component<transform::TransformComponent>())
            return default_transform;
        return o.component<transform::TransformComponent>();
    }
};

}  // namespace xc::xcal::object::policy