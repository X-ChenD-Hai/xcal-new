#pragma once
#include <cstddef>
#include <cstdint>
#include <format>
#include <stack>
#include <vector>

#include "xc/ecs2/comman/sparse_set.hpp"

namespace xc::ecs {

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

   private:
    entity_t entity_;
};

template <>
struct SparseSetValueTrait<Entity> {
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
}  // namespace xc::ecs

template <>
struct std::formatter<xc::ecs::Entity> {
    bool with_name = true;
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == 'n') {
            with_name = false;
            ++it;
        }
        return it;
    }
    auto format(const xc::ecs::Entity& e, format_context& ctx) const
        -> decltype(ctx.out()) {
        if (with_name)
            return std::format_to(ctx.out(), "Entity({}#{})", e.id(),
                                  e.version());
        else
            return std::format_to(ctx.out(), "{}#{}", e.id(), e.version());
    }
};
template <typename T, typename U>
struct std::formatter<std::vector<T, U>> {
    std::formatter<T> fmt{};
    constexpr auto parse(format_parse_context& ctx) { return fmt.parse(ctx); }
    auto format(const std::vector<T, U>& v, format_context& ctx) const
        -> decltype(ctx.out()) {
        if (!v.size()) return std::format_to(ctx.out(), "[]");
        std::format_to(ctx.out(), "[");
        for (size_t i = 0; i < v.size() - 1; ++i) {
            fmt.format(v[i], ctx);
            std::format_to(ctx.out(), ", ");
        }
        fmt.format(v.back(), ctx);
        return std::format_to(ctx.out(), "]");
    }
};