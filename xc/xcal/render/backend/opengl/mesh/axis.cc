#include "./axis.hpp"

#include "../openglloader.h"
xc::xcal::render::opengl::Line::Line(xcmath::vec3f direction) {
    _gl glGenBuffers(1, &vbo_);
    std::array<xcmath::vec3f, 4> data = {
        direction / 2,
        {1.0f, 0.0f, 0.0f},  //
        -direction / 2,
        {0.0f, 1.0f, 0.0f},  //
    };
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_);
    _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(xcmath::vec3f),
                     data.data(), _gl GL_STATIC_DRAW);
};

uint32_t xc::xcal::render::opengl::Line::vbo() const { return vbo_; };