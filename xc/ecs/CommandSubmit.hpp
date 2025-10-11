#pragma once
#include <vector>

#include "./Entity.hpp"
#include "./utils/traits.hpp"
namespace ecs {
class World;

enum class CommandAction {
    Empty = 0,
    CreateEntity,
    DestroyEntity,
    AttachComponent,
    SetComponent,
    RemoveComponents,
    CopyComponents,
};

struct Command {
    CommandAction action_ = CommandAction::Empty;
    virtual ~Command() {}
};
struct CreateEntityCommand : virtual public Command {
    std::vector<std::pair<component_t, void *>> components_;
};

class CommandSubmit {
    friend class World;
    std::vector<std::unique_ptr<Command>> commands_;

   protected:
    void execute(World &world);

   public:
    CommandSubmit() {}

    template <typename... Components>
    CommandSubmit *create_entity(Components &&...components);
};
template <typename... Components>
CommandSubmit *CommandSubmit::create_entity(Components &&...components) {
    auto command = std::make_unique<CreateEntityCommand>();
    command->action_ = CommandAction::CreateEntity;
    (
        [&]() {
            using Com = purge_t<Components>;
            command->components_.emplace_back(
                ComponentIdGenerator<Com>::get(),
                new Com(std::forward<Components>(components)));
        }(),
        ...);
    commands_.push_back(std::move(command));
    return this;
}
}  // namespace ecs
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