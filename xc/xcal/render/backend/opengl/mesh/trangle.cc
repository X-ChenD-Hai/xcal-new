#include "./trangle.hpp"

#include <print>

#include "../openglloader.h"
xc::xcal::render::opengl::Trangle::Trangle(xcmath::vec3f a, xcmath::vec3f b,
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
    auto layout = VertexLayout{{{
                                    .location = 0,
                                    .size = 3,
                                    .type = VertexAttributeType::Float,
                                    .normalized = false,
                                    .stride = 6 * sizeof(float),
                                    .offset = 0,
                                },
                                {
                                    .location = 1,
                                    .size = 3,
                                    .type = VertexAttributeType::Float,
                                    .normalized = false,
                                    .stride = 6 * sizeof(float),
                                    .offset = 3 * sizeof(float),
                                }}};
    layout.dump();
    std::print("Trangle init\n");
}
xc::xcal::render::opengl::MeshComponent
xc::xcal::render::opengl::Trangle::mesh_component() {
    return {
        .vao_id = vao,
        .vbo_id = vbo,
        .ebo_id = 0,
        .type = MeshType::TRIANGLES,
        .draw_count = 3,
        .draw_offset = 0,
    };
}
