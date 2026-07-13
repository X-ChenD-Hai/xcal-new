#pragma once
#include <cstdint>
#include <xc/ecs/types.hpp>
struct VAO {
    uint32_t vao;
};
struct VBO {
    uint32_t vbo;
    uint32_t count;
};

struct EBO {
    uint32_t vbo;
    uint32_t ebo;
    uint32_t count;
};

struct UBO {
    uint32_t ubo;
};

class Shader {};

struct BufferLayout {};

struct DrawDriver {
    static void init(ecs::World& world);
    DrawDriver();

    ~DrawDriver();

    void update();

   private:
    unsigned int vertex_shader_;
    unsigned int fragment_shader_;
    unsigned int shader_program_;
    unsigned int vao_, vbo_;
};
