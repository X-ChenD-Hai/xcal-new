#pragma once
#include <vector>
#include <print>
#include "./ComponentInfo.hpp"
#include "./Entity.hpp"
#include "./utils/traits.hpp"

namespace ecs {

class World;
class Entity;
class Querier {
    const World &world_;
    const std::vector<size_t> component_info_index_;

   public:
    Querier(const World &world,
            const std::vector<size_t> &component_infos_index)
        : world_(world), component_info_index_(component_infos_index) {}
    template <typename... Components>
    Querier query() const;
    std::vector<Entity> entities() const;
    void inset_info_index(std::vector<size_t> &component_info_index,
                          component_t component_id) const;
};
template <typename... Components>
Querier Querier::query() const {
    std::vector<size_t> component_info_index = component_info_index_;
    (inset_info_index(component_info_index,
                      ComponentIdGenerator<purge_t<Components>>::get()),
     ...);
    //  std::print("component_info_index: [");
    //  for (auto idx : component_info_index) {
    //     std::print("{}, ", idx);
    // }
    // std::print("]\n");
    return Querier{world_, component_info_index};
}
}  // namespace ecs