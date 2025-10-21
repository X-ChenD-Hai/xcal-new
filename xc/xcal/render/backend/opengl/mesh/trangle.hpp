#pragma once
#include <cstdint>
#include <render/backend/opengl/Mesh.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::render::opengl {

struct Trangle : public Mesh {
    uint32_t vbo_;
    Trangle(xcmath::vec3f a, xcmath::vec3f b, xcmath::vec3f c);
    VertexLayout layout() const override {
        return {{{
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
    };
    uint32_t vbo() const override;
    uint32_t draw_count() const override { return 3; };
    MeshType draw_type() const override { return MeshType::TRIANGLES; }
    std::string vertex_shader_path() const override { return "./res/vertex_color.vs"; }
    std::string fragment_shader_path() const override { return "./res/vertex_color.fs"; }
};
struct TranglePath : public Trangle {
    using Trangle::Trangle;
    MeshType draw_type() const override {
        return MeshType::LINE_LOOP;
    }
};
}  // namespace xc::xcal::render::opengl