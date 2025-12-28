#pragma once
#include <ecs/command/command.hpp>
#include <ecs/component_info.hpp>
#include <ecs/entity.hpp>
#include <ecs/utils/traits.hpp>
namespace ecs::command {

class DetachComponents : public Command {
   public:
    DetachComponents(Entity entity) : entity_(entity) {}

    template <typename... Component>
    DetachComponents& detach() {
        (components_.emplace_back(get_component_id<Component>()), ...);
        return *this;
    }
    CommandExecutePriority execute_priority() const noexcept override {
        return CommandExecutePriority::DELETE_COMPONENTS;
    };
    void execute(World& world) const override;

   private:
    Entity entity_;
    std::vector<component_t> components_{};
};

}  // namespace ecs::command
