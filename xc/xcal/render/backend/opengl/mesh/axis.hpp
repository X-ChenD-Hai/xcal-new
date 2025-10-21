#pragma once
#include <cstdint>
#include <render/backend/opengl/Mesh.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::render::opengl {

struct Line : public Mesh {
    uint32_t vbo_;
    Line(xcmath::vec3f direction);
    VertexLayout layout() const override {
        return {{{
            .size = 3,
            .type = VertexAttributeType::Float,
            .normalized = false,
            .stride = 3 * sizeof(float),
            .offset = 0,
            .vbo_id = vbo_,
        }}};
    };
    uint32_t draw_count() const override { return 2; };
    MeshType draw_type() const override { return MeshType::LINES; }
};

struct ParametricCurve : public Mesh {
    uint32_t vbo_, count_;
    ParametricCurve(const std::function<xcmath::vec3f(float)>& f, float start,
                    float end, int segments);
    VertexLayout layout() const override {
        return {{{
            .size = 3,
            .type = VertexAttributeType::Float,
            .normalized = false,
            .stride = 3 * sizeof(float),
            .offset = 0,
            .vbo_id = vbo_,
        }}};
    }
    uint32_t draw_count() const override { return count_; };
    MeshType draw_type() const override { return MeshType::LINE_STRIP; }
};

}  // namespace xc::xcal::render::opengl