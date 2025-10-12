#pragma once
#include <ecs/World.hpp>
#include <flags.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class TransformState : uint8_t {
    Dirty = 1 << 0,
    Visible = 1 << 1,
};

using TransformStateFlags = flags::Flags<TransformState>;

struct TransformComponent {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    TransformStateFlags state;

    void apply_transform(glm::mat4 &matrix) const;
};
struct TransformMatrixComponent {
    glm::mat4 matrix;
};
struct ShaderComponent {
    uint32_t program_id;
};
struct MeshComponent {
    uint32_t vao_id;
    uint32_t vbo_id;
    uint32_t ebo_id;
    uint32_t num_indices;
};

void update_transform_matrix(ecs::Querier q, ecs::ComponentAccessor a);