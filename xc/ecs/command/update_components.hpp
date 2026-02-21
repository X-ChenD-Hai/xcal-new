#pragma once
#include "../command/command.hpp"
#include "../entity.hpp"
#include "../utils/traits.hpp"
#include "../component_info.hpp"

namespace ecs::command {

class UpdateComponents : public Command {
   public:
    template <typename... Components>
    UpdateComponents(Entity entity, Components&&... components)
        : entity_(entity) {
        (update<Components>(std::forward<Components>(components)), ...);
    }

    template <typename Component, typename... Args>
        requires(std::is_constructible_v<Component, Args...>)
    UpdateComponents& update(Args&&... args) {
        using Com = purge_t<Component>;
        components_.emplace_back(ComponentIdGenerator<Com>::get(),
                                 new Com(std::forward<Args>(args)...));
        return *this;
    }
    template <typename Component1, typename Component2, typename... Components>
    UpdateComponents& update(Component1&& component1, Component2&& component2,
                             Components&&... components) {
        update<Component1>(component1);
        update<Component2>(component2);
        (update<Components>(components), ...);
        return *this;
    }

   protected:
    ExecutePriority execute_priority() const noexcept override {
        return ExecutePriority::MODIFY_COMPONENTS;
    };
    void execute(World& world) const override;

   private:
    Entity entity_;
    std::vector<std::pair<component_t, void*>> components_{};
};

}  // namespace ecs::command
