#pragma once
#include <cstddef>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <xc_assert.hpp>
namespace ecs {
class ResourceTable {
    struct ResourceDescribtor {
        size_t type_id = -1;
        bool is_trivially_destructible = true;
        uint32_t id = -1;
    };
    struct Pairhash {
        constexpr static auto hash_ = std::hash<size_t>{};
        size_t operator()(const std::pair<size_t, size_t> &p) const {
            return hash_(p.first) ^
                   (hash_(p.second) << (sizeof(size_t) * 8 / 2));
        }
    };
    std::unordered_map<size_t, void (*)(void *)>
        register_types_destroy_callbacks_;
    std::vector<void *> resources_;
    std::vector<ResourceDescribtor> describtors_;
    std::unordered_map<std::pair<size_t, size_t>, uint32_t, Pairhash>
        resource_map_;
    std::vector<std::function<void()>> asyn_create_tasks_;
    std::vector<size_t> asyn_release_id_;
    struct CreateResouse {
        void *(*create_func)(void *);
        ResourceDescribtor describtor;
    };

   public:
    template <typename T, typename Catgory = void, typename... Args>
        requires(std::is_constructible_v<T, Args...>)
    T *create_or_get(Args &&...args) {
        ResourceDescribtor describtor;
        if (auto it = resource_map_.find(
                {typeid(T).hash_code(), typeid(Catgory).hash_code()});
            it != resource_map_.end()) {
            if (resources_[it->second] != nullptr)
                return static_cast<T *>(resources_[it->second]);
            describtor = describtors_[it->second];
            describtor.id = it->second;
            return (T *)(resources_[it->second] =
                             new T(std::forward<Args>(args)...));
        }
        describtor = ResourceDescribtor{};
        if constexpr (!std::is_trivially_destructible_v<T>) {
            describtor.is_trivially_destructible = false;
            describtor.type_id = typeid(T).hash_code();
            if (!register_types_destroy_callbacks_.contains(
                    describtor.type_id)) {
                register_types_destroy_callbacks_[describtor.type_id] =
                    [](void *ptr) { delete static_cast<T *>(ptr); };
            }
        }
        describtor.id = static_cast<uint32_t>(resources_.size());
        resources_.emplace_back(new T(std::forward<Args>(args)...));
        describtors_.emplace_back(describtor);
        resource_map_[{typeid(T).hash_code(), typeid(Catgory).hash_code()}] =
            static_cast<uint32_t>(describtors_.size() - 1);

        return static_cast<T *>(resources_.back());
    }
    template <typename T, typename Catgory = void, typename... Args>
        requires(std::is_constructible_v<T, Args...>)
    T *async_create_or_get(Args &&...args) {
        if (auto it = resource_map_.find(
                {typeid(T).hash_code(), typeid(Catgory).hash_code()});
            it != resource_map_.end()) {
            if (resources_[it->second] != nullptr)
                return static_cast<T *>(resources_[it->second]);
        }
        asyn_create_tasks_.emplace_back(
            [args..., this]() { create_or_get<T, Catgory>(args...); });
        return nullptr;
    }
    template <typename T, typename Catgory = void>
    size_t resource_id() const {
        auto it = resource_map_.find(
            {typeid(T).hash_code(), typeid(Catgory).hash_code()});
        if (it == resource_map_.end()) return -1;
        return it->second;
    }
    template <typename T, typename Catgory = void>
    bool has_resource() const {
        if (auto it = resource_map_.find(
                {typeid(T).hash_code(), typeid(Catgory).hash_code()});
            it != resource_map_.end()) {
            return resources_[it->second];
        }
        return false;
    }

    template <typename T, typename Catgory = void>
    T *get_resource() const {
        auto it = resource_map_.find(
            {typeid(T).hash_code(), typeid(Catgory).hash_code()});
        if (it == resource_map_.end()) return nullptr;
        return static_cast<T *>(resources_[it->second]);
    }

    bool has_resource(size_t id) const {
        return id < resources_.size() && resources_[id] != nullptr;
    }

    void release_resource(size_t id) {
        XC_ASSERT(id < resources_.size());
        if (resources_[id] == nullptr) return;
        auto &describtor = describtors_[id];
        if (describtor.is_trivially_destructible) {
            std::destroy_at(resources_[describtor.id]);
        } else {
            register_types_destroy_callbacks_[describtor.type_id](
                resources_[describtor.id]);
        }
        resources_[describtor.id] = nullptr;
    }
    void async_release_resource(size_t id) {
        asyn_release_id_.emplace_back(id);
    }
    template <typename T, typename Catgory = void>
    void async_release_resource() {
        auto it = resource_map_.find(
            {typeid(T).hash_code(), typeid(Catgory).hash_code()});
        if (it == resource_map_.end()) return;
        async_release_resource(it->second);
    }

    template <typename T, typename Catgory = void>
    void release_resource() {
        auto it = resource_map_.find(
            {typeid(T).hash_code(), typeid(Catgory).hash_code()});
        if (it == resource_map_.end()) return;
        release_resource(it->second);
    }

    void do_async_create_tasks() {
        for (auto id : asyn_release_id_) {
            release_resource(id);
        }
        asyn_release_id_.clear();
        for (auto &task : asyn_create_tasks_) {
            task();
        }
        asyn_create_tasks_.clear();
    }

    ~ResourceTable() {
        for (auto &describtor : describtors_) {
            if (describtor.id >= resources_.size() &&
                resources_[describtor.id] == nullptr)
                continue;
            XC_ASSERT(describtor.id < resources_.size());
            if (describtor.is_trivially_destructible) {
                std::destroy_at(resources_[describtor.id]);
            } else {
                register_types_destroy_callbacks_[describtor.type_id](
                    resources_[describtor.id]);
            }
        }
    }
};

}  // namespace ecs
