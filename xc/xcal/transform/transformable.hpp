#pragma once
#include <crtp_base.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::transform {

template <class Derived, typename Policy>
class Transformable : public CrtpBase<Derived> {
    using CrtpBase<Derived>::self;

   public:
    inline const xcmath::vec3f& position() const {
        return Policy::transform(self().world(), self().entity()).position;
    }
    inline const xcmath::vec3f& rotation() const {
        return Policy::transform(self().world(), self().entity()).rotation;
    }
    inline const xcmath::vec3f& scale() const {
        return Policy::transform(self().world(), self().entity()).scale;
    }

    inline auto& set_position(const xcmath::vec3f& p) {
        Policy::set_pos(self().world(), self().entity(), p);
        return self();
    }
    inline auto& move(const xcmath::vec3f& delta) {
        Policy::set_pos(self().world(), self().entity(), position() + delta);
        return self();
    }
    inline auto& set_position(float x, float y, float z) {
        return set_position(xcmath::vec3f{x, y, z});
    }
    inline auto& move(float dx, float dy, float dz) {
        return move(xcmath::vec3f{dx, dy, dz});
    }

    // 旋转相关（单位：弧度）
    inline auto& set_rotation(const xcmath::vec3f& r) {
        Policy::set_rotation(self().world(), self().entity(), r);
        return self();
    }
    inline auto& rotate(const xcmath::vec3f& delta) {
        Policy::set_rotation(self().world(), self().entity(),
                             rotation() + delta);
        return self();
    }
    inline auto& set_rotation(float rx, float ry, float rz) {
        return set_rotation(xcmath::vec3f{rx, ry, rz});
    }
    inline auto& rotate(float drx, float dry, float drz) {
        return rotate(xcmath::vec3f{drx, dry, drz});
    }

    // 缩放相关
    inline auto& set_scale(const xcmath::vec3f& s) {
        Policy::set_scale(self().world(), self().entity(), s);
        return self();
    }
    inline auto& scale(const xcmath::vec3f& factor) {
        Policy::set_scale(self().world(), self().entity(), scale() * factor);
        return self();
    }
    inline auto& set_scale(float sx, float sy, float sz) {
        return set_scale(xcmath::vec3f{sx, sy, sz});
    }
    inline auto& scale(float fsx, float fsy, float fsz) {
        return scale(xcmath::vec3f{fsx, fsy, fsz});
    }
};

}  // namespace xc::xcal::transform