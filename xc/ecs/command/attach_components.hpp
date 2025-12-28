#pragma once
#include <ecs/command/command.hpp>
#include <ecs/component_info.hpp>
#include <ecs/entity.hpp>
#include <ecs/utils/traits.hpp>
namespace ecs::command {

class AttachComponents : public Command {
    Entity entity_;
    std::vector<std::pair<component_t, void*>> components_{};

   public:
    template <typename... Components>
    AttachComponents(Entity entity, Components&&... components)
        : entity_(entity) {
        (add<Components>(std::forward<Components>(components)), ...);
    }

    template <typename Component, typename... Args>
        requires(std::is_constructible_v<Component, Args...>)
    AttachComponents& add(Args&&... args) {
        using Com = purge_t<Component>;
        components_.emplace_back(ComponentIdGenerator<Com>::get(),
                                 new Com(std::forward<Args>(args)...));
        return *this;
    }
    template <typename Component1, typename Component2, typename... Components>
    AttachComponents& add(Component1&& component1, Component2&& component2,
                          Components&&... components) {
        add<Component1>(component1);
        add<Component2>(component2);
        (add<Components>(components), ...);
        return *this;
    }
    CommandExecutePriority execute_priority() const noexcept override {
        return CommandExecutePriority::APPEND_COMPONENTS;
    };
    void execute(World& world) const override;
};

}  // namespace ecs::command
