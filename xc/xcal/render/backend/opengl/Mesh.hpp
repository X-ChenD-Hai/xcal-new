#pragma once
#include <cstdint>
#include <ecs/ComponentAccessor.hpp>
#include <string>
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
    uint32_t vbo_id;
    uint32_t ebo_id;
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
    uint32_t location;
    uint32_t size;
    VertexAttributeType type;
    uint32_t normalized;
    uint32_t stride;
    uint32_t offset;
    bool operator==(const VertexAttribute& other) const = default;
    void dump() const;
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

    static std::unordered_map<VertexLayout, uint32_t> vaos;
    VertexArrayObject(const VertexLayout& layout);
};

struct Mesh {
    virtual VertexLayout layout() const = 0;
    virtual uint32_t vbo() const = 0;
    virtual uint32_t ebo() const { return 0; }; 
    virtual uint32_t draw_count()  const= 0;
    virtual MeshType draw_type() const = 0;
    virtual uint32_t draw_offset() const { return 0; };
    virtual uint32_t shader() const { return 0; };
    virtual xc::xcal::render::opengl::MeshComponent mesh_component() const;
    virtual std::string vertex_shader_path() const { return ""; };
    virtual std::string fragment_shader_path() const { return ""; };
};

void render_mesh(ecs::Querier q, ecs::ComponentAccessor a);
}  // namespace xc::xcal::render::opengl


template <>
struct std::hash<xc::xcal::render::opengl::VertexAttribute> {
    static constexpr std::hash<size_t> hasher{};
    constexpr std::size_t operator()(
        const xc::xcal::render::opengl::VertexAttribute& k) const {
        return hasher(k.location) ^ hasher(k.size) ^
               hasher(static_cast<size_t>(k.type)) ^ hasher(k.normalized) ^
               hasher(k.stride) ^ hasher(k.offset);
    }
};
template <>
struct std::hash<xc::xcal::render::opengl::VertexLayout> {
    static constexpr std::hash<xc::xcal::render::opengl::VertexAttribute>
        hasher{};
    constexpr std::size_t operator()(
        const xc::xcal::render::opengl::VertexLayout& k) const {
        std::size_t seed = 0;
        for (const auto& attr : k.attributes) {
            seed ^= hasher(attr);
        }
        return seed;
    }
};