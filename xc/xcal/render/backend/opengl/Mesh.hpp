#pragma once
#include <ecs/ComponentAccessor.hpp>
#include <cstdint>



namespace xc::xcal::render::opengl {
struct MeshComponent {
    uint32_t vao_id;
    uint32_t vbo_id;
    uint32_t ebo_id;
    uint32_t draw_count;
    uint32_t draw_offset;
};
void render_mesh(ecs::Querier q, ecs::ComponentAccessor a);
}  // namespace xc::xcal::render::opengl