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
    uint32_t draw_count() const override { return 2; };
    MeshType draw_type() const override { return MeshType::LINES; }
    std::string vertex_shader_path() const override {
        return "./res/vertex_color.vs";
    }
    std::string fragment_shader_path() const override {
        return "./res/vertex_color.fs";
    }
};

struct ParametricCurve : public Mesh {
    uint32_t vbo_;
    ParametricCurve(const std::function<xcmath::vec3f(float)>& f, float start,
                    float end, int segments);
    VertexLayout layout() const override {
        return {{{
            .location = 0,
            .size = 3,
            .type = VertexAttributeType::Float,
            .normalized = false,
            .stride = 3 * sizeof(float),
            .offset = 0,
        }}};
    }
    std::string vertex_shader_path() const override {
        return "./res/single_color.vs";
    }
    std::string fragment_shader_path() const override {
        return "./res/single_color.fs";
    }
};

}  // namespace xc::xcal::render::opengl