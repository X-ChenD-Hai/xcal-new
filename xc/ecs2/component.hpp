#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "xc/common/type_map.hpp"
#include "xc/ecs2/comman/sparse_set.hpp"
#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/entity.hpp"
#include "xc/ecs2/markers.hpp"

namespace xc::ecs {

class BaseComponentPool {
   public:
    BaseComponentPool(size_t id) : id_{id} {}

    size_t id() const { return id_; }
    bool dirty() const { return dirty_; }
    void set_dirty(bool dirty) const { dirty_ = dirty; }

   public:
    virtual std::string_view component_name() const = 0;
    virtual size_t vtl_size() const = 0;
    virtual const std::vector<Entity>& vtl_entities() const = 0;
    virtual void vtl_clear() = 0;
    virtual void vtl_erase(Entity entity) = 0;
    virtual bool vtl_contains(Entity entity) const = 0;
    virtual ~BaseComponentPool() = 0;

   private:
    size_t id_;
    mutable bool dirty_{true};
};

inline BaseComponentPool::~BaseComponentPool() = default;

template <typename T>
class ComponentPoolSlot {
   public:
    ComponentPoolSlot(Entity e, const T& c) : entity_{e}, component_{c} {}
    Entity entity() const { return entity_; }
    T& component() { return component_; }
    const T& component() const { return component_; }
    T* operator->() { return &component_; }
    const T* operator->() const { return &component_; }
    T& operator*() { return component_; }
    const T& operator*() const { return component_; }

   private:
    Entity entity_;
    T component_;
};

template <typename T>
struct SparseSetValueTrait<ComponentPoolSlot<T>>
    : public SparseSetValueTrait<Entity> {
    using slot_t = ComponentPoolSlot<T>;
    static id_t id_of(const slot_t& value) { return value.entity().id(); }
    static version_t version_of(const slot_t& value) {
        return value.entity().version();
    }
};

template <typename T>
class ComponentPool : public BaseComponentPool {
   public:
    using index_t = size_t;
    using value_t = size_t;
    using BaseComponentPool::BaseComponentPool;

    void insert(Entity entity, const T& component) {
        set_dirty(true);
        components_.insert(ComponentPoolSlot<T>{entity, component});
    }
    T& get(Entity entity) { return components_[entity.id()].component(); }
    const T& get(Entity entity) const {
        return components_[entity.id()].component();
    }
    bool contains(Entity entity) const {
        return components_.contains(entity.id(), entity.version());
    }
    void erase(Entity entity) {
        set_dirty(true);
        components_.erase(entity.id(), entity.version());
    }
    void clear() { components_.clear(); }
    size_t size() const { return components_.size(); }
    const std::vector<Entity>& entities() const {
        if (dirty()) {
            set_dirty(false);
            entities_.resize(components_.size(), Entity(0, 0));
            auto idx = 0;
            for (auto c : components_) {
                entities_[idx++] = c.entity();
            }
        }
        return entities_;
    }
    virtual void vtl_erase(Entity entity) override { erase(entity); }
    virtual bool vtl_contains(Entity entity) const override {
        return contains(entity);
    }

   public:
    ~ComponentPool() override = default;
    std::string_view component_name() const override {
        return typeid(T).name();
    }
    void vtl_clear() override { components_.clear(); }
    size_t vtl_size() const override { return components_.size(); }
    const std::vector<Entity>& vtl_entities() const override {
        return entities();
    }

   public:
    auto begin() { return components_.begin(); }
    auto end() { return components_.end(); }
    auto begin() const { return components_.begin(); }
    auto end() const { return components_.end(); }

   private:
    SparseSet<ComponentPoolSlot<T>> components_{};
    mutable std::vector<Entity> entities_{};
};

template <typename... T>
class ComponentQuery;

class ComponentRegistry {
    friend struct std::formatter<xc::ecs::ComponentRegistry>;

   public:
    template <typename... T>
    class ComponentQuery;
    using pool_ptr = std::unique_ptr<BaseComponentPool>;
    struct fill_value {
        operator pool_ptr() const noexcept { return nullptr; }
    };
    ComponentRegistry() = default;
    ~ComponentRegistry() = default;

    template <typename T>
    ComponentPool<T>& pool() {
        return *static_cast<ComponentPool<T>*>(pools_.data<T>().get());
    }
    template <typename T>
    const ComponentPool<T>& pool() const {
        return *static_cast<ComponentPool<T>*>(pools_.data<T>().get());
    }
    template <typename T>
    size_t pool_id() const {
        return pools_.type_id<T>();
    }
    template <typename... T>
    ComponentRegistry& regist() {
        (pool_ptrs_.emplace_back(
             (pools_.data<T>() =
                  pool_ptr{new ComponentPool<T>{pools_.type_id<T>()}})
                 .get()),
         ...);
        return *this;
    }
    template <typename T>
    bool has() {
        return pools_.data<T>() != nullptr;
    }
    template <typename T>
    size_t size() {
        return pool<T>().size();
    }
    template <typename T>
    ComponentRegistry& insert(Entity entity, T component) {
        pool<T>().insert(entity, component);
        return *this;
    }
    template <typename T>
    T& get(Entity entity) {
        return pool<T>().get(entity);
    }
    template <typename T>
    const T& get(Entity entity) const {
        return pool<T>().get(entity);
    }
    template <typename... Ts>
        requires(sizeof...(Ts) > 1)
    std::tuple<Ts&...> get(Entity entity) {
        return std::tuple<Ts&...>(pool<Ts>().get(entity)...);
    }
    template <typename... T>
        requires(sizeof...(T) > 1)
    std::tuple<const T&...> get(Entity entity) const {
        return std::tuple<T&...>(pool<T>().get(entity)...);
    }
    template <typename T, typename... Ts>
    bool contains(Entity entity) const {
        return pool<T>().contains(entity) &&
               (pool<Ts>().contains(entity) && ...);
    }
    template <typename T, typename... Ts>
    bool any_uncontains(Entity entity) const {
        return !pool<T>().contains(entity) ||
               (!pool<Ts>().contains(entity) || ...);
    }
    template <typename T, typename... Ts>
    bool all_uncontains(Entity entity) const {
        return !pool<T>().contains(entity) &&
               (!pool<Ts>().contains(entity) && ...);
    }
    template <typename T, typename... Ts>
    bool any_contains(Entity entity) const {
        return pool<T>().contains(entity) ||
               (pool<Ts>().contains(entity) || ...);
    }
    template <typename T, typename... Ts>
    void erase(Entity entity) {
        pool<T>().erase(entity);
        (pool<Ts>().erase(entity), ...);
    }
    void erase(Entity entity) {
        for (auto& p : pools_) {
            if (!p->vtl_contains(entity)) continue;
            p->vtl_erase(entity);
        }
    }
    template <typename T, typename... Ts>
    void clear() {
        pool<T>().clear();
        (pool<Ts>().clear(), ...);
    }
    void clear() {
        for (auto& p : pools()) p->vtl_clear();
    }
    const std::vector<BaseComponentPool*>& pools() const { return pool_ptrs_; }

   private:
    TypeMap<std::unique_ptr<BaseComponentPool>, fill_value> pools_{
        fill_value{}};
    std::vector<BaseComponentPool*> pool_ptrs_{};
};

class ComponentQueryCachePool {
   public:
    using cache_item_t = std::tuple<bool, std::vector<Entity>>;

    ComponentQueryCachePool(ComponentRegistry& registry)
        : registry_(registry) {}

    template <typename T>
    cache_item_t& cache() {
        return caches_.data<T>();
    }
    template <typename T>
    const cache_item_t& cache() const {
        return caches_.data<T>();
    }
    template <typename T>
    const std::vector<Entity>& entities() const {
        return std::get<1>(caches_.data<T>());
    }

    template <typename T>
    std::vector<Entity>& entities() {
        return std::get<1>(caches_.data<T>());
    }

    template <typename T>
    bool dirty() const {
        return std::get<0>(caches_.data<T>());
    }
    template <typename T>
    void set_dirty(bool dirty) {
        std::get<0>(caches_.data<T>()) = dirty;
    }
    void clear() {
        for (auto& [dirty, p] : caches_) p.clear();
    }
    inline ComponentRegistry& registry() { return registry_; }
    inline const ComponentRegistry& registry() const { return registry_; }

    template <typename... T>
        requires(!traits::is_specialized_v<ComponentQuery, T> && ...)
    ComponentQuery<T...> query() {
        return {*this};
    }
    template <typename T>
        requires(traits::is_specialized_v<ComponentQuery, T>)
    T query() {
        return {*this};
    }

   private:
    ComponentRegistry& registry_;
    TypeMap<cache_item_t> caches_{std::make_tuple(true, std::vector<Entity>())};
};

template <typename... T>
using base_query_t = traits::repack_t<
    collect_all_markers_with_default_t<
        Read, traits::template_record<ReadWrite, ExcludeAny, ExcludeAll>, T...>,
    ComponentQuery>;
template <typename... T>
class ComponentQuery : public base_query_t<T...> {
   public:
    using base_query_t<T...>::base_query_t;
};

template <typename... R, typename... Rw, typename... Eany, typename... Eall>
class ComponentQuery<Read<R...>, ReadWrite<Rw...>, ExcludeAny<Eany...>,
                     ExcludeAll<Eall...>> {
   public:
    using read_t = Read<R...>;
    using read_write_t = ReadWrite<Rw...>;
    using exclude_any_t = ExcludeAny<Eany...>;
    using exclude_all_t = ExcludeAll<Eall...>;
    using cache_tag_t = CacheTag<R..., Rw..., exclude_all_t, exclude_any_t>;
    static constexpr size_t read_comp_count = sizeof...(R);
    static constexpr size_t read_write_comp_count = sizeof...(Rw);
    static constexpr size_t e_any_comp_count = sizeof...(Eany);
    static constexpr size_t e_all_comp_count = sizeof...(Eall);
    static constexpr size_t require_comp_count =
        read_comp_count + read_write_comp_count;
    static constexpr bool single_comp_query =
        (require_comp_count == 1) && !e_all_comp_count && !e_any_comp_count;

   public:
    ComponentQuery(ComponentQueryCachePool& cache_pool)
        : cache_pool_(cache_pool) {}
    ~ComponentQuery() = default;
    const std::vector<Entity>& query() {
        if (cache_pool_.dirty<cache_tag_t>())
            return cache_pool_.entities<cache_tag_t>();
        auto& registry = cache_pool_.registry();
        auto& entities = cache_pool_.entities<cache_tag_t>();
        if constexpr (single_comp_query) {
            return registry.pool<R...>().entities();
        } else {
            entities.clear();
            const BaseComponentPool* main = nullptr;
            if constexpr (require_comp_count > 1) {
                std::array<const BaseComponentPool*, require_comp_count> pools =
                    {nullptr};
                auto idx = 0;
                ((pools[idx++] = &registry.pool<R>()), ...);
                ((pools[idx++] = &registry.pool<Rw>()), ...);
                main = *std::min_element(
                    pools.begin(), pools.end(), [](auto a, auto b) {
                        return a->vtl_size() < b->vtl_size();
                    });
            } else {
                main = &registry.pool<R..., Rw...>();
            }
            for (auto e : main->vtl_entities()) {
                bool matched = registry.contains<R..., Rw...>(e);
                if constexpr (e_any_comp_count) {
                    matched = matched && registry.any_uncontains<Eany...>(e);
                }
                if constexpr (e_all_comp_count) {
                    matched = matched && registry.all_uncontains<Eall...>(e);
                }
                if (matched) entities.push_back(e);
            }
            return entities;
        }
    }
    template <std::invocable<const R&..., Rw&...> Fn>
    void each(Fn&& fn) {
        auto& registry = cache_pool_.registry();
        if constexpr (single_comp_query) {
            if constexpr (read_comp_count) {
                for (const auto& s : registry.pool<R...>()) {
                    std::forward<Fn>(fn)(s.component());
                }
            } else if constexpr (read_write_comp_count) {
                for (auto& s : registry.pool<Rw...>()) {
                    std::forward<Fn>(fn)(s.component());
                }
            }
        } else {
            for (auto e : query()) {
                std::forward<Fn>(fn)(registry.get<R>(e)...,
                                     registry.get<Rw>(e)...);
            }
        }
    }
    template <typename Fn>
        requires(std::is_invocable_r_v<bool, Fn, const R&..., Rw&...>)
    std::vector<Entity> filter(Fn&& fn) {
        std::vector<Entity> entities;
        auto& registry = cache_pool_.registry();
        if constexpr (single_comp_query) {
            if constexpr (read_comp_count) {
                for (const auto& s : registry.pool<R...>()) {
                    if (std::forward<Fn>(fn)(s.component()))
                        entities.push_back(s.entity());
                }
            } else if constexpr (read_write_comp_count) {
                for (auto& s : registry.pool<Rw...>()) {
                    if (std::forward<Fn>(fn)(s.component()))
                        entities.push_back(s.entity());
                }
            }
        } else {
            for (auto e : query()) {
                if (std::forward<Fn>(fn)(registry.get<R>(e)...,
                                         registry.get<Rw>(e)...))
                    entities.push_back(e);
            }
        }
        return entities;
    }
    static std::vector<size_t> read_component_id_list(
        const ComponentRegistry& registry_) {
        std::vector<size_t> ids;
        (ids.push_back(registry_.pool_id<R>()), ...);
        (ids.push_back(registry_.pool_id<Rw>()), ...);
        return ids;
    }
    static std::vector<size_t> write_component_id_list(
        const ComponentRegistry& registry_) {
        std::vector<size_t> ids;
        (ids.push_back(registry_.pool_id<Rw>()), ...);
        return ids;
    }

   private:
    ComponentQueryCachePool& cache_pool_;
};

}  // namespace xc::ecs
template <>
struct std::formatter<xc::ecs::ComponentRegistry> {
    constexpr auto parse(format_parse_context& ctx) { return ++ctx.begin(); }
    auto format(const xc::ecs::ComponentRegistry& registry,
                std::format_context& ctx) const {
        auto o = std::format_to(ctx.out(), "ComponentRegistry(pools=[");
        bool pushed = false;
        std::vector<xc::ecs::BaseComponentPool*> pools{};
        for (auto& p : registry.pools_)
            if (p) pools.push_back(p.get());
        for (size_t i = 0; i < pools.size(); ++i) {
            if (i) o = std::format_to(o, ", ");
            o = std::format_to(o, "{}(id={})", pools[i]->component_name(),
                               pools[i]->id());
        }
        o = format_to(o, "])");
        return o;
    }
};
