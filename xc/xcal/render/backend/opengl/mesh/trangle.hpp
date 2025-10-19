#pragma once
#include <cstdint>
#include <render/backend/opengl/Mesh.hpp>
#include <xcmath/xcmath.hpp>

namespace xc::xcal::render::opengl {
struct Mesh {
    virtual VertexLayout layout() = 0;
    virtual uint32_t vbo() = 0;
    virtual uint32_t ebo() { return 0; };
    virtual uint32_t draw_count() = 0;
    virtual MeshType draw_type() = 0;
    virtual uint32_t draw_offset() { return 0; };
    virtual uint32_t shader() { return 0; };
    virtual xc::xcal::render::opengl::MeshComponent mesh_component();
};

struct Trangle {
    uint32_t vao, vbo, shader;
    Trangle(xcmath::vec3f a, xcmath::vec3f b, xcmath::vec3f c);
    xc::xcal::render::opengl::MeshComponent mesh_component();
};
}  // namespace xc::xcal::render::opengl