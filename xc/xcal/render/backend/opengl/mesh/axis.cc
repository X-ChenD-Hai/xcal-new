#include "./axis.hpp"

#include "../openglloader.h"
xc::xcal::render::opengl::Line::Line(xcmath::vec3f direction) {
    std::array<xcmath::vec3f, 2> data = {
        direction / 2,
        -direction / 2,
    };
    _gl glGenBuffers(1, &vbo_);
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_);
    _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(xcmath::vec3f),
                     data.data(), _gl GL_STATIC_DRAW);
    // _gl glBindBuffer(_gl GL_ARRAY_BUFFER, 0);
    set_color({0, 1, 0, 1});
};

xc::xcal::render::opengl::ParametricCurve::ParametricCurve(
    const std::function<xcmath::vec3f(float)>& f, float start, float end,
    int segments) {
    std::vector<xcmath::vec3f> data;
    data.reserve(segments + 1);
    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        xcmath::vec3f p = f(t);
        data.push_back(p);
    }
    _gl glGenBuffers(1, &vbo_);
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_);
    _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(xcmath::vec3f),
                     data.data(), _gl GL_STATIC_DRAW);
    // _gl glBindBuffer(_gl GL_ARRAY_BUFFER, 0);
    count_ = data.size();
    set_color({0, 1, 1, 1});
}
