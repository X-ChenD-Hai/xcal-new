#pragma once
#include <print>
#include <vector>

#include "./Entity.hpp"
#include "./utils/traits.hpp"

namespace ecs {
class World;



struct Command {
    virtual void execute(World &world) = 0;
    virtual ~Command() {}
};
struct CreateEntityCommand : public Command {
    std::vector<std::pair<component_t, void *>> components_;
    template <typename... Components>
    CreateEntityCommand(Components &&...components) {
        (add<components>(std::forward<components>(components)), ...);
    }
    template <typename Component, typename... Args>
    CreateEntityCommand &add(Args &&...args) {
        using Com = purge_t<Component>;
        components_.emplace_back(ComponentIdGenerator<Com>::get(),
                                 new Com(std::forward<Args>(args)...));
    }
    virtual void execute(World &world);
};

class CommandSubmit {
    friend class World;
    std::vector<std::unique_ptr<Command>> commands_;

   protected:
    void execute_then_clear(World &world);

   public:
    CommandSubmit() {}

    template <typename Command, typename... Args>
    CommandSubmit &submit(Args &&...args) {
        commands_.emplace_back(
            std::make_unique<Command>(std::forward<Args>(args)...));
    }

    template <typename... Components>
    CommandSubmit &create_entity(const Components &...components);
};

template <typename... Components>
CommandSubmit &CommandSubmit::create_entity(const Components &...components) {
    auto command = std::make_unique<CreateEntityCommand>();
    // std::println("CreateEntityCommand size: {}",sizeof...(components));
    (
        [&]() {
            using Com = purge_t<Components>;
            command->components_.emplace_back(ComponentIdGenerator<Com>::get(),
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
