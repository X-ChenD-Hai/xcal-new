#pragma once
#include <id_generator.hpp>
#include <sparse_list.hpp>
#include <xc_assert.hpp>
namespace ecs {

class World;
class Entity final {
    friend class World;
    friend class CommandSubmit;

   public:
    using entity_t = uint64_t;
    using id_t = uint32_t;
    using version_t = uint32_t;

   private:
    static constexpr entity_t id_mask = static_cast<id_t>(-1);
    static constexpr entity_t version_mask =
        static_cast<entity_t>(static_cast<version_t>(-1)) << 32;
    entity_t entity_;

   public:
    Entity(id_t id, version_t version)
        : entity_(id | (entity_t)version << 32) {}

   public:
    constexpr entity_t id() const noexcept { return entity_ & id_mask; };
    constexpr entity_t version() const noexcept {
        return (entity_ & version_mask) >> 32;
    };
    constexpr entity_t entity() const noexcept { return entity_; };
    constexpr bool is_valid() const noexcept {
        return id() != id_mask && version() != version_mask;
    }

   public:
    Entity(const Entity &) = default;
    Entity(Entity &&) = default;
    Entity &operator=(const Entity &) = default;
    Entity &operator=(Entity &&) = default;
};
class ComponentInfo;
template <typename Component_>
using ComponentIdGenerator =
    ThreadSaftyIdGeneratorTemplate<ComponentInfo,
                                   uint32_t>::Generator<Component_>;
using ComponentCounter =
    ThreadSaftyIdGeneratorTemplate<ComponentInfo, uint32_t>;
}  // namespace ecs