#include "./trangle.hpp"

#include <print>

#include "../openglloader.h"
xc::xcal::render::opengl::Trangle::Trangle(xcmath::vec3f a, xcmath::vec3f b,
                                           xcmath::vec3f c) {
    _gl glGenBuffers(1, &vbo_);
    std::array<xcmath::vec3f, 6> data = {
        a, {1.0f, 0.0f, 0.0f},  //
        b, {0.0f, 1.0f, 0.0f},  //
        c, {0.0f, 0.0f, 1.0f},  //
    };
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_);
    _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(xcmath::vec3f),
                     data.data(), _gl GL_STATIC_DRAW);
}

uint32_t xc::xcal::render::opengl::Trangle::vbo() const {
    return vbo_;
};