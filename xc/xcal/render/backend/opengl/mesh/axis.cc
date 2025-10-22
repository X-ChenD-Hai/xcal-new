#include "./axis.hpp"

#include <print>

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
xc::xcal::render::opengl::ParametricSurface::ParametricSurface(
    const std::function<xcmath::vec3f(float, float)>& f, float u_start,
    float u_end, int u_segments, float v_start, float v_end, int v_segments) {
    // 1. 生成顶点 ----------------------------------------------------------
    const int u_verts = u_segments + 1;
    const int v_verts = v_segments + 1;
    std::vector<xcmath::vec3f> vbo_data;
    vbo_data.reserve(u_verts * v_verts);

    const float u_step = (u_end - u_start) / u_segments;
    const float v_step = (v_end - v_start) / v_segments;

    for (int i = 0; i < u_verts; ++i) {
        float u = u_start + i * u_step;
        for (int j = 0; j < v_verts; ++j) {
            float v = v_start + j * v_step;
            vbo_data.push_back(f(u, v));
        }
    }

    _gl glGenBuffers(1, &vbo_);
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo_);
    _gl glBufferData(_gl GL_ARRAY_BUFFER,
                     vbo_data.size() * sizeof(xcmath::vec3f), vbo_data.data(),
                     _gl GL_STATIC_DRAW);

    // 2. 生成索引（每格 2 个三角形） ----------------------------------------
    const int tri_count = u_segments * v_segments * 2;
    std::vector<uint32_t> ebo_data;
    ebo_data.reserve(tri_count * 3);

    auto idx = [&](int i, int j) -> uint32_t {
        return static_cast<uint32_t>(i * v_verts + j);
    };

    for (int i = 0; i < u_segments; ++i) {
        for (int j = 0; j < v_segments; ++j) {
            uint32_t a = idx(i, j);
            uint32_t b = idx(i, j + 1);
            uint32_t c = idx(i + 1, j + 1);
            uint32_t d = idx(i + 1, j);

            // 第一个三角形
            ebo_data.push_back(a);
            ebo_data.push_back(b);
            ebo_data.push_back(c);
            // 第二个三角形
            ebo_data.push_back(a);
            ebo_data.push_back(c);
            ebo_data.push_back(d);
        }
    }

    _gl glGenBuffers(1, &ebo_);
    _gl glBindBuffer(_gl GL_ELEMENT_ARRAY_BUFFER, ebo_);
    _gl glBufferData(_gl GL_ELEMENT_ARRAY_BUFFER,
                     ebo_data.size() * sizeof(uint32_t), ebo_data.data(),
                     _gl GL_STATIC_DRAW);

    count_ = static_cast<_gl GLsizei>(ebo_data.size());  // 用于 glDrawElements

    set_color({0.0f, 0.5f, 0.5f, 0.5f});
}