#pragma once

#include <cstddef>
#include <memory>

#include "xc/common/type_map.hpp"

namespace xc::ecs {

class ComponentRegistry {
   public:
    using res_ptr_t = std::unique_ptr<void, void (*)(void*)>;
    ComponentRegistry() = default;
    template <typename T, typename... Args>
    ComponentRegistry& create(Args&&... args) {
        resources_.data<T>() = res_ptr_t(new T(std::forward<Args>(args)...),
                                         [](void* p) { delete (T*)p; });
        return *this;
    }
    template <typename T>
    T& get() {
        return (T*)resources_.data<T>().get();
    }
    template <typename T>
    const T& get() const {
        return (T*)resources_.data<T>().get();
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
    TypeMap<res_ptr_t, nullptr_t> resources_{nullptr};
};

}  // namespace xc::ecs