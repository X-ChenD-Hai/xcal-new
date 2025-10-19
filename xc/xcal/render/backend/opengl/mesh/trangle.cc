#include "./trangle.hpp"
#include <print>
#include "../openglloader.h"

xc::xcal::render::opengl::Trangle::Trangle(xcmath::vec3f a,
                                                   xcmath::vec3f b,
                                                   xcmath::vec3f c) {
    _gl glGenVertexArrays(1, &vao);
    _gl glGenBuffers(1, &vbo);
    _gl glBindVertexArray(vao);
    std::array<xcmath::vec3f, 6> data = {
        a, {1.0f, 0.0f, 0.0f},  //
        b, {0.0f, 1.0f, 0.0f},  //
        c, {0.0f, 0.0f, 1.0f},  //
    };
    _gl glBindBuffer(_gl GL_ARRAY_BUFFER, vbo);
    _gl glBufferData(_gl GL_ARRAY_BUFFER, data.size() * sizeof(xcmath::vec3f),
                     data.data(), _gl GL_STATIC_DRAW);
    _gl glEnableVertexAttribArray(0);
    _gl glVertexAttribPointer(0, 3, _gl GL_FLOAT, _gl GL_FALSE,
                              6 * sizeof(float), (void*)0);
    _gl glEnableVertexAttribArray(1);
    _gl glVertexAttribPointer(1, 3, _gl GL_FLOAT, _gl GL_FALSE,
                              6 * sizeof(float), (void*)(3 * sizeof(float)));
    std::print("Trangle init\n");
}
xc::xcal::render::opengl::MeshComponent
xc::xcal::render::opengl::Trangle::mesh_component() {
    return {
        .vao_id = vao,
        .vbo_id = vbo,
        .ebo_id = 0,
        .draw_count = 3,
        .draw_offset = 0,
    };
}
