#pragma once
#include <concepts>
#include <memory>
#include <utility>
#include <variant>

#include "./command/command.hpp"
#include "./entity.hpp"

namespace ecs {
class World;
class CommandSubmit {
    friend class World;

    using command_ptr =
        std::variant<std::unique_ptr<command::Command>, command::Command*>;

   protected:
    void execute_then_clear(World& world);

   public:
    CommandSubmit() {}

    CommandSubmit& submit(ecs::command::Command* cmd) {
        commands_[(uint32_t)cmd->execute_priority()].push_back(cmd);
        return *this;
    }
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

   private:
    std::array<std::vector<command_ptr>,
               (size_t)command::ExecutePriority::AFTER_ALL + 1>
        commands_;
};
}  // namespace ecs
