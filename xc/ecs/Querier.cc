#include "./Querier.hpp"
#include "./World.hpp"

std::vector<ecs::Entity> ecs::Querier::entities() const {
    if (component_info_index_.empty()) return std::vector<Entity>();
    auto idx = *std::min_element(
        component_info_index_.begin(), component_info_index_.end(),
        [&](size_t a, size_t b) {
            return world_.component_infos_[a].entities_.size() <
                   world_.component_infos_[b].entities_.size();
        });
    auto result = std::vector<Entity>();
    result.reserve(world_.component_infos_[idx].entities_.size());
    for (auto entity : world_.component_infos_[idx].entities_) {
        result.push_back(Entity(entity, 0));
    }

    auto size = result.size();
    result.erase(
        std::remove_if(
            result.begin(), result.end(),
            [&](const Entity &entity) {
                for (auto comp_idx : component_info_index_) {
                    if (!world_.component_infos_[comp_idx].has_entity(entity)) {
                        return true;  // 移除
                    }
                }
                return false;  // 保留
            }),
        result.end());

    return result;
}
void ecs::Querier::inset_info_index(std::vector<size_t> &component_info_index,
                               component_t component_id) const {
    if (auto idx = world_.component2pool_map_.get_index(component_id);
        idx < world_.component_infos_.size()) {
        component_info_index.push_back(idx);
    }
}
