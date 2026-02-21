#include "./command_submit.hpp"

#include <variant>

#include "./world.hpp"

void ecs::CommandSubmit::execute_then_clear(World& world) {
    for (auto& commands : commands_) {
        for (auto& command : commands)
            std::visit([&](auto& cmd) { cmd->execute(world); }, command);
        commands.clear();
    }
}
