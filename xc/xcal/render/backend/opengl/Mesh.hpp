#pragma once
#include <cstdint>
#include <ecs/component_accessor.hpp>
#include <string>

#include "./shader.hpp"

namespace ecs {
class ResourceTable;
}

namespace xc::xcal::render::opengl {

enum class MeshType : uint32_t {
    LINE_STRIP,
    LINE_LOOP,
    LINES,
    POINTS,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    QUADS,
    QUAD_STRIP,
    POLYGON,
};
struct MeshComponent {
    uint32_t vao_id;
    bool use_element_buffer;
    MeshType type;
    uint32_t draw_count;
    uint32_t draw_offset;
};
enum class VertexAttributeType : uint8_t {
    Float,
    Double,
    Int,
    UInt,
};
struct VertexAttribute {
    uint32_t size;
    VertexAttributeType type;
    uint32_t normalized;
    uint32_t stride;
    uint32_t offset;
    uint32_t vbo_id;
    bool operator==(const VertexAttribute& other) const = default;
    void dump(uint32_t location) const;
};

struct VertexLayout {
    std::vector<VertexAttribute> attributes;
    constexpr VertexLayout(const std::vector<VertexAttribute>& attributes)
        : attributes(attributes) {};
    bool operator==(const VertexLayout& other) const = default;
    void dump() const;
};
struct VertexArrayObject {
    uint32_t id;

    VertexArrayObject(const VertexLayout& layout, uint32_t ebo = 0);
};
struct ShaderProgram;
struct Mesh {
    ShaderComponent shader_program;

    void set_color(const xcmath::vec4f& color);
    void use_vertex_color();

    virtual VertexLayout layout() const = 0;
    virtual uint32_t ebo() const { return 0; }
    virtual uint32_t draw_count() const = 0;
    virtual MeshType draw_type() const = 0;
    virtual uint32_t draw_offset() const { return 0; }
    virtual uint32_t shader() const { return 0; }
    virtual xc::xcal::render::opengl::MeshComponent mesh_component() const;
};

void render_mesh(ecs::Querier q, ecs::ComponentAccessor a,
                 ecs::ResourceTable& resources);
}  // namespace xc::xcal::render::opengl
