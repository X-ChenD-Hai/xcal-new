#pragma once
#include <cstdint>
#include <xcmath/xcmath.hpp>
namespace xc::xcal::render::opengl {

struct UniformBuffer {
    uint32_t ubo;

    UniformBuffer();
    void update(const xcmath::mat4f& mat);
    void bind();
};

}  // namespace xc::xcal::render::opengl