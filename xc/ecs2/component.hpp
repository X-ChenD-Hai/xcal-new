#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "xc/common/type_map.hpp"
#include "xc/ecs2/comman/sparse_set.hpp"
#include "xc/ecs2/entity.hpp"

namespace xc::ecs {
class BaseComponentPool {
   public:
    BaseComponentPool(size_t id) : id_{id} {}
    virtual ~BaseComponentPool() = 0;

    size_t id() const { return id_; }
    virtual size_t size() const = 0;
    virtual std::string_view component_name() const = 0;
    virtual const std::vector<Entity>& entities() const = 0;
    virtual bool contains(Entity entity) const = 0;
    bool dirty() const { return dirty_; }
    void set_dirty(bool dirty) const { dirty_ = dirty; }

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
    bool contains(Entity entity) const override {
        return components_.contains(entity.id(), entity.version());
    }
    void erase(Entity entity) {
        set_dirty(true);
        components_.erase(entity);
    }
    void clear() { components_.clear(); }
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

class ComponentRegistry {
   public:
    template <typename... T>
    friend class ComponentQuery;
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
    template <typename T>
    bool contains(Entity entity) const {
        return pool<T>().contains(entity);
    }
    template <typename T>
    void erase(Entity entity) {
        pool<T>().erase(entity);
    }
    template <typename T>
    void clear() {
        pool<T>().clear();
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
template <typename... T>
class ComponentQuery {
   public:
    static constexpr size_t comp_count = sizeof...(T);
    ComponentQuery(ComponentRegistry& reg) : registry_(reg) {}
    ~ComponentQuery() = default;
    const std::vector<Entity>& query() {
        if constexpr (comp_count <= 1) {
            return registry_.pool<T...>().entities();
        } else {
            if (entities_.size()) return entities_;
            std::array<const BaseComponentPool*, sizeof...(T)> pools = {
                nullptr};
            auto idx = 0;
            ((pools[idx++] = &registry_.pool<T>()), ...);
            auto it = std::min_element(
                pools.begin(), pools.end(),
                [](auto a, auto b) { return a->size() < b->size(); });
            auto main = *it;
            std::swap(pools[comp_count - 1], *it);
            for (auto e : main->entities()) {
                bool matched = true;
                for (size_t i = 0; i < comp_count - 1; i++) {
                    if (!pools[i]->contains(e)) {
                        matched = false;
                        break;
                    }
                }
                if (matched) entities_.push_back(e);
            }
            return entities_;
        }
    }
    template <typename Fn>
        requires(std::is_invocable_v<Fn, T&...>)
    void each(Fn fn) {
        if constexpr (comp_count == 1) {
            for (auto& s : registry_.pool<T...>()) {
                fn(s.component());
            }
        } else {
            for (auto e : query(registry_)) {
                fn(registry_.get<T>(e)...);
            }
        }
    }
    std::vector<Entity> entities_{};
    ComponentRegistry& registry_;
};

}  // namespace xc::ecs