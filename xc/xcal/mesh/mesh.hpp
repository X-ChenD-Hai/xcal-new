#pragma once

#include <cstdint>
#include <vector>
namespace xc::xcal {

enum class MeshAction { Create, Update, Delete };

struct Mesh {
    uint32_t id;
    uint32_t vertex_id;
    uint32_t index_id;
    MeshAction action;
};

struct Buffer {
    uint32_t id;
    uint32_t size;
    uint32_t usage;
};

class MeshManager {
    private:
        std::vector<Mesh> meshes;
    public:

};

}  // namespace xc::xcal