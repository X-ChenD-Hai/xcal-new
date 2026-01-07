#pragma once
#include <common/flags.hpp>
#include <ecs/tyoes.hpp>
#include <xcmath/xcmath.hpp>

namespace xcal::transform {
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

    xcmath::mat4f to_mat() const;

    
};
struct TransformMatrixComponent {
    xcmath::mat4f matrix{xcmath::mat4f::eye()};
};
class TransformPlugin {
    friend class ecs::World;

   protected:
    static void install(ecs::World& world);
    static void run(ecs::World& world);
};
}  // namespace xcal::transform