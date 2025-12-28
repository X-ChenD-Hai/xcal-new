#pragma once
#include <ecs/command/command.hpp>

#include "./entity.hpp"

namespace ecs {
class World;
class CommandSubmit {
    friend class World;

    std::array<std::vector<std::unique_ptr<command::Command>>,
               (size_t)command::CommandExecutePriority::AFTER_ALL + 1>
        commands_;

   protected:
    void execute_then_clear(World& world);

   public:
    CommandSubmit() {}

    template <typename Command, typename... Args>
        requires(std::is_constructible_v<Command, Args...>)
    CommandSubmit& submit(Args&&... args) {
        auto cmd = std::make_unique<Command>(std::forward<Args>(args)...);
        commands_[(uint32_t)cmd->execute_priority()].push_back(std::move(cmd));
        return *this;
    }
};
}  // namespace ecs
