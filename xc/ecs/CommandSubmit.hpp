#pragma once
#include <vector>
#include <print>
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
    CommandSubmit &create_entity(const Components &...components);
};


template <typename... Components>
CommandSubmit &CommandSubmit::create_entity(const Components &...components) {
    auto command = std::make_unique<CreateEntityCommand>();
    // std::println("CreateEntityCommand size: {}",sizeof...(components));
    command->action_ = CommandAction::CreateEntity;
    (
        [&]() {
            using Com = purge_t<Components>;
            command->components_.emplace_back(
                ComponentIdGenerator<Com>::get(),
                new Com(components));
            // std::printf("CreateEntityCommand: %d\n",
            //             ComponentIdGenerator<Com>::get());
        }(),
        ...);
    commands_.push_back(std::move(command));

    // std::terminate();
    return *this;
}
}  // namespace ecs
