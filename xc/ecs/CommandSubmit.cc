#include "./CommandSubmit.hpp"

#include "./World.hpp"

void CommandSubmit::execute(World &world) {
    for (auto &command : commands_) {
        if (command->action_ == CommandAction::CreateEntity) {
            auto create_entity_command =
                dynamic_cast<CreateEntityCommand *>(command.get());
            auto entity = Entity{(Entity::id_t)world.entities_.size(), 0};
            world.entities_.emplace_back(entity);
            for (auto &component : create_entity_command->components_) {
                XC_ASSERT(world.component2pool_map_.has_value(component.first));
                auto index =
                    world.component2pool_map_.get_index(component.first);
                auto &component_info = world.component_infos_[index];
                auto pool_index = component_info.pool_index_;
                world.pools_[pool_index].emplace_back(component.second,
                                                      component_info.deleter_);
                component_info.add_entity(entity);
            }
        }
    }
    commands_.clear();
}
// template <typename... Components>
// CommandSubmit *CommandSubmit::create_entity(Components &&...components) {
//     auto entity = Entity{(Entity::id_t)world_.entities_.size(), 0};
//     world_.entities_.emplace_back(entity);
//     (
//         [&]() {
//             using Com = purge_t<Components>;
//             XC_ASSERT(world_.component2pool_map_.has_value(
//                 ComponentIdGenerator<Com>::get()));
//             auto cp = ComponentIdGenerator<Com>::get();
//             auto index = world_.component2pool_map_.get_index(cp);
//             auto &component_info = world_.component_infos_[index];
//             auto pool_index = component_info.pool_index_;
//             world_.pools_[pool_index].emplace_back(
//                 (void *)new
//                 purge_t<Com>(std::forward<Components>(components)),
//                 component_info.deleter_);
//             component_info.add_entity(entity);
//         }(),
//         ...);
//     return this;
// }