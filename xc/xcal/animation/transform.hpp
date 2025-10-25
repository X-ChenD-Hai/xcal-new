#pragma once
#include <crtp_base.hpp>
#include <xcal/transform/transformable.hpp>

#include "../object/policy/transform.hpp"
#include "./animation.hpp"
#include "./translate.hpp"
#include "xcal/transform/transform.hpp"

namespace xc::xcal::animation {
namespace policy {
class Transform : public object::policy::Transform {
    template <class Derived, typename Policy>
    friend class transform::Transformable;
    static inline void set_pos(animation::Animation& a,
                               const xcmath::vec3f& p) {
        a.set_component<TranslateAnimationComponent>({
            .animation = a.animation_component(),
            .start_pos = transform(a).position,
            .end_pos = p,
        });
    }
};
}  // namespace policy

class Transform
    : public Animation,
      public transform::Transformable<Transform, policy::Transform> {
   private:
    transform::TransformComponent to_transform_component_;

   public:
    Transform(float start_time, float end_time,
              interpolation_function::interpolation_function interpolation_func,
              const object::Object& object)
        : Animation(object, start_time, end_time, interpolation_func) {}
};
}  // namespace xc::xcal::animation