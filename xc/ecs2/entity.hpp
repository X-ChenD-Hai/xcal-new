#include <cstdint>
#include <format>
#include <stack>
#include <xc/ecs2/comman/sparse_set.hpp>

namespace ecs {

class Entity {
   public:
    static constexpr size_t IdBitWidth = 49;
    using entity_t = uint64_t;
    using id_t = uint64_t;
    using version_t = uint32_t;
    static constexpr id_t IdMask = (id_t(1) << IdBitWidth) - 1;

    Entity(id_t id, version_t version)
        : entity_(id | (entity_t(version) << IdBitWidth)) {}

    id_t id() const { return entity_ & IdMask; }
    version_t version() const { return entity_ >> IdBitWidth; }

    std::string to_string() {
        return std::format("id = {}, version = {}", id(), version());
    }

   private:
    entity_t entity_;
};

template <>
struct SparseSetValueInfo<Entity> {
    using id_t = uint64_t;
    using version_t = uint32_t;
    static id_t id_of(const Entity& value) { return value.id(); }
    static version_t version_of(const Entity& value) { return value.version(); }
    static constexpr id_t InvalidId = std::numeric_limits<id_t>::max();
};

class EntityFactory {
   public:
    Entity spawn() {
        if (free_list_.empty()) {
            return Entity(next_id_++, 0);
        }
        auto e = free_list_.top();
        free_list_.pop();
        return e;
    }
    void free(Entity e) { free_list_.emplace(e.id(), e.version() + 1); }

   private:
    std::stack<Entity> free_list_{};
    size_t next_id_{0};
};
}  // namespace ecs