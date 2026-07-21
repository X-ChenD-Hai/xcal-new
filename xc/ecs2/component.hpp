#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "xc/common/type_map.hpp"
#include "xc/ecs2/comman/sparse_set.hpp"
#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/entity.hpp"

namespace xc::ecs {

class BaseComponentPool {
   public:
    BaseComponentPool(size_t id) : id_{id} {}
    virtual ~BaseComponentPool() = 0;

    size_t id() const { return id_; }
    bool dirty() const { return dirty_; }
    void set_dirty(bool dirty) const { dirty_ = dirty; }

   public:
    virtual size_t size() const = 0;
    virtual std::string_view component_name() const = 0;
    virtual const std::vector<Entity>& entities() const = 0;
    virtual void clear() = 0;

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

    std::string_view component_name() const override {
        return typeid(T).name();
    }
    void clear() override { components_.clear(); }
    size_t size() const override { return components_.size(); }
    ~ComponentPool() override = default;
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
        components_.erase(entity);
    }
    const std::vector<Entity>& entities() const override {
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
    template <typename... T>
    void regist() {
        ((pools_.data<T>() =
              pool_ptr{new ComponentPool<T>{pools_.type_id<T>()}}),
         ...);
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
    void insert(Entity entity, T component) {
        pool<T>().insert(entity, component);
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
    template <typename T, typename... Ts>
    void clear() {
        pool<T>().clear();
        (pool<Ts>().clear(), ...);
    }
    void clear() {
        for (auto& p : pools_) {
            if (p) {
                p->clear();
            }
        }
    }

    std::string to_string() {
        std::string ret = "[";
        for (auto& pool : pools_) {
            ret +=
                std::format("{}({}), ", pool->component_name(), pool->size());
        }
        ret.pop_back();
        ret.pop_back();
        ret += "]";
        return ret;
    }

   private:
    TypeMap<std::unique_ptr<BaseComponentPool>, fill_value> pools_{
        fill_value{}};
};

template <typename... U>
struct ExcludeAny;

template <typename... U>
struct Include;

template <typename... T>
using query_include_t =
    details::collect_marker_t<true, Include,
                              details::template_record<ExcludeAny>, T...>;
template <typename... T>
using query_exclude_any_t =
    details::collect_marker_t<false, ExcludeAny, details::template_record<>,
                              T...>;

template <typename... T>
class ComponentQuery
    : public ComponentQuery<query_include_t<T...>, query_exclude_any_t<T...>> {
   public:
    using ComponentQuery<query_include_t<T...>,
                         query_exclude_any_t<T...>>::ComponentQuery;
};

template <typename... T, typename... U>
class ComponentQuery<Include<T...>, ExcludeAny<U...>> {
   public:
    using include_t = Include<T...>;
    using exclude_any_t = ExcludeAny<U...>;
    static constexpr size_t inc_comp_count = sizeof...(T);
    static constexpr size_t exc_comp_count = sizeof...(U);
    ComponentQuery(ComponentRegistry& reg) : registry_(reg) {}
    ~ComponentQuery() = default;
    const std::vector<Entity>& query() {
        if constexpr (inc_comp_count <= 1 && !exc_comp_count) {
            return registry_.pool<T...>().entities();
        } else {
            if (entities_.size()) return entities_;
            const BaseComponentPool* main = nullptr;
            if constexpr (inc_comp_count > 1) {
                std::array<const BaseComponentPool*, sizeof...(T)> pools = {
                    nullptr};
                auto idx = 0;
                ((pools[idx++] = &registry_.pool<T>()), ...);
                main = *std::min_element(
                    pools.begin(), pools.end(),
                    [](auto a, auto b) { return a->size() < b->size(); });
            } else {
                main = &registry_.pool<T...>();
            }

            for (auto e : main->entities()) {
                bool matched = registry_.contains<T...>(e);
                if constexpr (exc_comp_count > 0) {
                    matched = matched && registry_.any_uncontains<U...>(e);
                }
                if (matched) entities_.push_back(e);
            }
            return entities_;
        }
    }
    template <std::invocable<T&...> Fn>
    void each(Fn&& fn) {
        if constexpr (inc_comp_count == 1 && !exc_comp_count) {
            for (auto& s : registry_.pool<T...>()) {
                std::forward<Fn>(fn)(s.component());
            }
        } else {
            for (auto e : query()) {
                std::forward<Fn>(fn)(registry_.get<T>(e)...);
            }
        }
    }

   private:
    std::vector<Entity> entities_{};
    ComponentRegistry& registry_;
};

}  // namespace xc::ecs