#pragma once
#include <ecs/command/command.hpp>
#include <ecs/world.hpp>
namespace ecs::command {
template <auto system>
class System : public Command {
   public:
    System(ExecutePriority priority = ExecutePriority::AFTER_ALL)
        : priority_(priority) {}

   protected:
    virtual ExecutePriority execute_priority() const noexcept {
        return priority_;
    }
    virtual void execute(World& world) const { world.run_system<system>(); }

   private:
    ExecutePriority priority_;
};
}  // namespace ecs::command
