#pragma once
#include <cstdint>
#include <flags.hpp>
#include <xcmath/xcmath.hpp>
#include <ecs/Querier.hpp>
#include <ecs/ComponentAccessor.hpp>

namespace xc::xcal::transform {
enum class TransformState : uint8_t {
    Dirty = 1 << 0,
    Visible = 1 << 1,
};
using TransformStateFlags = flags::Flags<TransformState>;
struct TransformComponent {
    xcmath::vec3f position{0.0f, 0.0f, 0.0f};
    xcmath::vec3f scale{1.0f, 1.0f, 1.0f};
    xcmath::vec3f rotation{0.0f, 0.0f, 0.0f};
    TransformStateFlags state{TransformState::Dirty, TransformState::Visible};

    xcmath::mat4f transform_matrix() const;
};
struct TransformMatrixComponent {
    xcmath::mat4f matrix{xcmath::mat4f::eye()};
};
void update_transform_matrix(ecs::Querier q, ecs::ComponentAccessor a);
}  // namespace xc::xcal::transform
