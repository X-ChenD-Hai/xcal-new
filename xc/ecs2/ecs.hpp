#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

#include "xc/common/sparse_list.hpp"

namespace xc::ecs {

struct Position {
    float x, y, z;
};
namespace ecs::traits {
template <typename T>
constexpr bool is_soa_v = (std::is_trivially_move_constructible_v<T> &&
                           std::is_trivially_destructible_v<T>);
}

namespace details {
template <size_t I>
struct Log2 {
    static constexpr size_t value = Log2<I / 2>::value + 1;
    static constexpr size_t mantissa = I - (1 << value);
};
template <>
struct Log2<1> {
    static constexpr size_t value = 0;
    static constexpr size_t mantissa = 1;
};
template <>
struct Log2<0> {
    static constexpr size_t mantissa = 0;
};

template <typename T>
struct SlotInfo {
    using index_t = size_t;
    static constexpr size_t index_t_size = sizeof(index_t);
    static constexpr size_t object_size = sizeof(T);
    static constexpr size_t min_slot_size =
        (object_size > index_t_size ? object_size : index_t_size);
    static constexpr size_t slot_grade =
        Log2<min_slot_size>::value + (Log2<min_slot_size>::mantissa ? 1 : 0);
    static constexpr size_t slot_size = 1 << slot_grade;
    static constexpr size_t data_size = slot_size;
};

}  // namespace details

template <typename T>
class SoASlot {
    using info = details::SlotInfo<T>;

   private:
    union {
        std::byte data[info::data_size];
        size_t next_{0};
    };
};
class Entity {
   public:
    using index_t = uint64_t;
    using version_t = uint64_t;
    using entity_t = uint64_t;

    static constexpr size_t VersionBitCount = 16;
    static constexpr size_t IndexBitCount =
        sizeof(entity_t) * 8 - VersionBitCount;
    static constexpr entity_t IndexMask = (1ull << IndexBitCount) - 1;

    Entity(index_t index, version_t version)
        : entity_(index | (version << IndexBitCount)) {}

    index_t id() const noexcept { return entity_ & IndexMask; }
    version_t version() const noexcept { return entity_ >> IndexBitCount; }
    void set_version(version_t version) noexcept {
        entity_ &= IndexMask;
        entity_ |= version << IndexBitCount;
    }

   private:
    entity_t entity_;
};

template <typename T>
class ComponentPool {
   public:
    using index_t = size_t;
    using value_t = size_t;
    size_t cell_index(Entity e) { return entities_.get_index(e); };
    void add_entity(Entity entity) { entities_.insert(entity); }
    void remove_entity(Entity entity) { entities_.remove(entity.id()); }
    bool has_entity(Entity entity) const { return entities_.has_value(entity); }

   private:
    std::vector<SoASlot<T>> slots_{};
    SparseList<Entity, size_t, 32> entities_{};
};

template <size_t max_grade = 8>
class SoAPool {
    SoAPool() {}

   private:
};

enum class CommandGrade : uint8_t {
    Delete,
    Append,
    Update,
};
class EntitySet {
    struct EntityIterator {};

   private:
};

struct Command {
    CommandGrade grade;
    EntitySet entities;
};

class CommandBuffer {};

class World {
   public:
   private:
};
}  // namespace xc::ecs