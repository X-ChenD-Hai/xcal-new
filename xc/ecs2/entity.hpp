#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <mutex>
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
};

class EntityFactory {
   public:
    Entity spawn() {
        {
            std::lock_guard lk{mtx_};
            if (!free_list_.empty()) {
                Entity e{free_list_.top().id(), free_list_.top().version() + 1};
                free_list_.pop();
                return e;
            }
        }
        return next_entity();
    }
    std::vector<Entity> spawn(size_t count) {
        std::vector<Entity> res{};
        res.reserve(count);
        ++count;
        {
            std::lock_guard lk{mtx_};
            while (--count && !free_list_.empty()) {
                auto e = free_list_.top();
                res.emplace_back(e.id(), e.version() + 1);
                free_list_.pop();
            }
        }
        while (--count) {
            res.emplace_back(next_entity());
        }

        return res;
    }
    void free(Entity e) {
        std::lock_guard lk{mtx_};
        free_list_.push(e);
    }
    void free(std::vector<Entity>& e) {
        std::lock_guard lk{mtx_};
        for (auto& e : e) {
            free_list_.push(e);
        }
    }

   protected:
    Entity next_entity() {
        return Entity{next_id_.fetch_add(1, std::memory_order_relaxed), 0};
    }

   private:
    std::stack<Entity> free_list_{};
    std::atomic<size_t> next_id_{0};
    std::mutex mtx_{};
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