#pragma once
#include <functional>
#include <xc/ecs/command/command.hpp>

namespace ecs::command {
class Function : public Command {
   public:
    Function(std::function<void()> cmd,
             ExecutePriority priority = ExecutePriority::AFTER_ALL)
        : func_{cmd}, priority_(priority) {}

   protected:
    virtual ExecutePriority execute_priority() const noexcept {
        return priority_;
    }
    virtual void execute(World& world) const { func_(); }

   private:
    std::function<void()> func_;
    ExecutePriority priority_;
};
}  // namespace ecs::command
