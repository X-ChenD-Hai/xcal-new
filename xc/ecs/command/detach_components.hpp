#pragma once
#include "../command/command.hpp"
#include "../component_info.hpp"
#include "../entity.hpp"
#include "../utils/traits.hpp"
namespace ecs::command {

class DetachComponents : public Command {
   public:
    DetachComponents(Entity entity) : entity_(entity) {}

    template <typename... Component>
    DetachComponents& detach() {
        (components_.emplace_back(get_component_id<Component>()), ...);
        return *this;
    }
    ExecutePriority execute_priority() const noexcept override {
        return ExecutePriority::DELETE_COMPONENTS;
    };
    void execute(World& world) const override;

   private:
    Entity entity_;
    std::vector<component_t> components_{};
};

}  // namespace ecs::command
