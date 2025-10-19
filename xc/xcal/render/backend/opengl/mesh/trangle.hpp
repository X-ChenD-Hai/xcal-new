#pragma once
#include <cstdint>
#include <xcmath/xcmath.hpp>
#include <render/backend/opengl/Mesh.hpp>
namespace xc::xcal::render::opengl {
    struct Trangle {
        uint32_t vao, vbo, shader;
        Trangle(xcmath::vec3f a, xcmath::vec3f b, xcmath::vec3f c);
        xc::xcal::render::opengl::MeshComponent mesh_component();
    };
}