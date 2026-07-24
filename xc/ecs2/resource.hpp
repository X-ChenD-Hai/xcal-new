#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include "xc/common/type_map.hpp"
#include "xc/ecs2/comman/traits.hpp"

namespace xc::ecs {

class ResourceRegistry {
   public:
    using res_ptr_t = std::unique_ptr<void, void (*)(void*)>;

    struct fill_type {
        operator res_ptr_t() const { return {nullptr, nullptr}; };
    };
    ResourceRegistry() = default;
    template <typename T, typename... Args>
    ResourceRegistry& create(Args&&... args) {
        assert(!has<T>());
        resources_.data<T>() = res_ptr_t(new T(std::forward<Args>(args)...),
                                         [](void* p) { delete (T*)p; });
        return *this;
    }
    template <typename T>
    T& get() {
        return *(T*)resources_.data<T>().get();
    }
    template <typename T>
    const T& get() const {
        return *(T*)resources_.data<T>().get();
    }
    template <typename T>
    bool has() const {
        if (resources_.capacity() <= resources_.type_id<T>()) return false;
        return resources_.data<T>() != nullptr;
    }
    template <typename T>
    size_t resource_id() const {
        return resources_.type_id<T>();
    }

   private:
    TypeMap<res_ptr_t, fill_type> resources_{{}};
};
template <typename... T>
class ResourceAccessor {
   public:
    using resource_t = traits::type_record<T...>;
    using writable_t =
        traits::remove_if_t<resource_t, traits::predicate<std::is_const>>;
    using readable_t = traits::type_record<std::decay_t<T>...>;

    ResourceAccessor(ResourceRegistry& registry) : registry_(registry) {}
    template <typename U>
    auto get() -> traits::enable_if_t<
        traits::contains_if_v<writable_t, traits::same_as<U>>, U&> {
        return registry_.get<std::decay_t<U>>();
    }
    template <typename U>
    auto get() const -> traits::enable_if_t<
        traits::contains_if_v<readable_t, traits::same_as<U>>, const U&> {
        return registry_.get<std::decay_t<U>>();
    }
    template <typename U>
    auto get() const -> traits::enable_if_t<
        std::is_const_v<U> &&
            traits::contains_if_v<readable_t, traits::same_as<std::decay_t<U>>>,
        const U&> {
        return registry_.get<std::decay_t<U>>();
    }
    static std::vector<size_t> write_resource_id_list(
        const ResourceRegistry& reg) {
        std::vector<size_t> idx{};
        idx.reserve(traits::size_of_v<writable_t>);
        [&]<typename... U>(traits::type_record<U...>*) {
            (idx.push_back(reg.resource_id<std::decay_t<U>>()), ...);
        }((writable_t*)nullptr);
    };
    static std::vector<size_t> read_resource_id_list(
        const ResourceRegistry& reg) {
        std::vector<size_t> idx{};
        idx.reserve(traits::size_of_v<writable_t>);
        [&]<typename... U>(traits::type_record<U...>*) {
            (idx.push_back(reg.resource_id<std::decay_t<U>>()), ...);
        }((readable_t*)nullptr);
    };

   private:
    ResourceRegistry& registry_;
};
}  // namespace xc::ecs