#include "./command_submit.hpp"

#include "./world.hpp"

void ecs::CommandSubmit::execute_then_clear(World &world) {
    for (auto &commands : commands_) {
        for (auto &command : commands) command->execute(world);
        commands.clear();
    }
}
