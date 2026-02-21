#pragma once
#include <cstdint>
#include <xc/xcal/render/backend/opengl/mesh.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::render::opengl {

struct Trangle : public Mesh {
    uint32_t vbo_;
    Trangle(xcmath::vec3f a, xcmath::vec3f b, xcmath::vec3f c);
    VertexLayout layout() const override {
        return {{{
                     .size = 3,
                     .type = VertexAttributeType::Float,
                     .normalized = false,
                     .stride = 6 * sizeof(float),
                     .offset = 0,
                     .vbo_id = vbo_,
                 },
                 {
                     .size = 3,
                     .type = VertexAttributeType::Float,
                     .normalized = false,
                     .stride = 6 * sizeof(float),
                     .offset = 3 * sizeof(float),
                     .vbo_id = vbo_,
                 }}};
    };
    uint32_t draw_count() const override { return 3; };
    MeshType draw_type() const override { return MeshType::TRIANGLES; }
};
struct TranglePath : public Trangle {
    using Trangle::Trangle;
    MeshType draw_type() const override { return MeshType::LINE_LOOP; }
};
}  // namespace xc::xcal::render::opengl