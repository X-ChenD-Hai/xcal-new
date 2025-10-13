#pragma once
#include <ecs/World.hpp>
#include <flags.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace xc::xcal {
namespace event {
struct FrameResize {
    uint32_t width;
    uint32_t height;
};
struct CameraChanged {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
};
struct CameraProjectionChanged {
    float fov;
    float aspect;
    float near;
    float far;
};

struct ViewMatrixUpdate {
    glm::mat4 view_matrix;
};
struct ProjectionMatrixUpdate {
    glm::mat4 projection_matrix;
};
}  // namespace event

enum class TransformState : uint8_t {
    Dirty = 1 << 0,
    Visible = 1 << 1,
};

using TransformStateFlags = flags::Flags<TransformState>;

struct TransformComponent {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    TransformStateFlags state{TransformState::Dirty, TransformState::Visible};

    void apply_transform(glm::mat4 &matrix) const;
};
struct TransformMatrixComponent {
    glm::mat4 matrix{1.0f};
};
struct ShaderComponent {
    uint32_t program_id;
};
struct MeshComponent {
    uint32_t vao_id;
    uint32_t vbo_id;
    uint32_t ebo_id;
    uint32_t draw_count;
    uint32_t draw_offset;
};

class ShaderManager {
    std::vector<std::string> shaders_;
};

void update_transform_matrix(ecs::Querier q, ecs::ComponentAccessor a);
void render_mesh(ecs::Querier q, ecs::ComponentAccessor a,
                 ShaderManager &shader_manager);
}  // namespace xc::xcal