#pragma once
#include <concepts>
#include <ecs/command/command.hpp>
#include <memory>
#include <utility>

#include "./entity.hpp"

namespace ecs {
class World;
class CommandSubmit {
    friend class World;

    std::array<std::vector<std::unique_ptr<command::Command>>,
               (size_t)command::ExecutePriority::AFTER_ALL + 1>
        commands_;

   protected:
    void execute_then_clear(World& world);

   public:
    CommandSubmit() {}

    CommandSubmit& submit(std::unique_ptr<ecs::command::Command>&& cmd) {
        commands_[(uint32_t)cmd->execute_priority()].push_back(std::move(cmd));
        return *this;
    }

    template <typename Command, typename... Args>
        requires(std::is_constructible_v<Command, Args...> &&
                 std::derived_from<Command, command::Command>)
    CommandSubmit& submit(Args&&... args) {
        auto cmd = std::make_unique<Command>(std::forward<Args>(args)...);
        commands_[(uint32_t)cmd->execute_priority()].push_back(std::move(cmd));
        return *this;
    }

    template <typename... Command>
        requires(std::derived_from<Command, command::Command> && ...)
    CommandSubmit& submit(Command&... cmd) {
        (commands_[(uint32_t)cmd.execute_priority()].emplace_back(
             new Command(std::move(cmd))),
         ...);
        return *this;
    }
};
}  // namespace ecs
