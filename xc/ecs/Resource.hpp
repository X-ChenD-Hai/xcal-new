#pragma once
#include <TypeMap.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <xc_assert.hpp>

namespace ecs {
using Cell_ = std::unique_ptr<void, std::function<void(void *)>>;
class Cell {};

class ResourceManager {
    static constexpr uint32_t INVALID_RESOURCE_ID = uint32_t(-1);
    TypeMap<uint32_t> resource_id_map_{INVALID_RESOURCE_ID};
    std::vector<Cell_> resources_{};

   public:
    template <typename Resource, typename... Args>
    ResourceManager &add(Args &&...args) noexcept {
        if (resource_id_map_.data<Resource>() == INVALID_RESOURCE_ID) {
            auto idx = resources_.size();
            resource_id_map_.data<Resource>() = idx;
            resources_.emplace_back(
                (void *)(new Resource(std::forward<Args>(args)...)),
                [](void *ptr) { delete static_cast<Resource *>(ptr); });
        } else {
            std::cerr << "Resource already exists" << std::endl;
        }
        return *this;
    };
    template <typename Resource>
    ResourceManager &add(Resource *resource) noexcept {
        if (resource_id_map_.data<Resource>() == INVALID_RESOURCE_ID) {
            resource_id_map_.data<Resource>() = resources_.size();
            resources_.emplace_back((void *)resource, [](void *ptr) {});
        } else {
            std::cerr << "Resource already exists" << std::endl;
        }
        return *this;
    };
    template <typename Resource>
    ResourceManager &remove() noexcept {
        auto idx = resource_id_map_.data<Resource>();
        if (idx != INVALID_RESOURCE_ID) {
            resource_id_map_.data<Resource>() = INVALID_RESOURCE_ID;
            resources_[idx].reset();
        } else {
            std::cerr << "Resource does not exist" << std::endl;
        }
        return *this;
    };
    template <typename Resource>
    Resource *try_get() noexcept {
        auto idx = resource_id_map_.data<Resource>();
        if (idx != INVALID_RESOURCE_ID) {
            return static_cast<Resource *>(resources_[idx].get());
        } else {
            return nullptr;
        }
    };
    template <typename Resource>
    const Resource *try_get() const noexcept {
        auto idx = resource_id_map_.data<Resource>();
        if (idx != INVALID_RESOURCE_ID) {
            return static_cast<const Resource *>(resources_[idx].get());
        } else {
            return nullptr;
        }
    };
    template <typename Resource>
    Resource &get() noexcept {
        XC_ASSERT(resource_id_map_.data<Resource>() < resources_.size());
        return *(Resource *)resources_[resource_id_map_.data<Resource>()].get();
    }
    template <typename Resource>
    const Resource &get() const noexcept {
        XC_ASSERT(resource_id_map_.data<Resource>() < resources_.size());
        return *(const Resource *)resources_[resource_id_map_.data<Resource>()]
                    .get();
    }
    template <typename Resource>
    bool has() const noexcept {
        return resource_id_map_.data<Resource>() != INVALID_RESOURCE_ID;
    };
    template <typename Resource>
    uint32_t id() const noexcept {
        return resource_id_map_.data<Resource>();
    }
    size_t size() const noexcept { return resources_.size(); }
};

}  // namespace ecs