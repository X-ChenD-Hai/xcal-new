#pragma once
#include <cstdint>
#include <render/backend/opengl/Mesh.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::render::opengl {
struct Mesh {
    virtual VertexLayout layout() const = 0;
    virtual uint32_t vbo() const = 0;
    virtual uint32_t ebo() const { return 0; };
    virtual uint32_t draw_count()  const= 0;
    virtual MeshType draw_type() const = 0;
    virtual uint32_t draw_offset() const { return 0; };
    virtual uint32_t shader() const { return 0; };
    virtual xc::xcal::render::opengl::MeshComponent mesh_component() const {
        return {
            .vao_id = VertexArrayObject(layout()).id,
            .vbo_id = vbo(),
            .ebo_id = ebo(),
            .type = draw_type(),
            .draw_count = draw_count(),
            .draw_offset = draw_offset(),
        };
    }
};

struct Trangle : public Mesh {
    uint32_t vbo_, shader;
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
};
struct TranglePath : public Trangle {
    using Trangle::Trangle;
    MeshType draw_type() const override {
        std::cout << "draw_type" << std::endl;
        return MeshType::LINE_LOOP;
    }
};
}  // namespace xc::xcal::render::opengl