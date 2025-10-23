#pragma once
#include <print>
#include <vector>

#include "./entity.hpp"
#include "./utils/traits.hpp"

namespace ecs {
class World;
enum class CommandExecutePriority : uint16_t {
    BEFORE_ALL = 0,
    APPEND_COMPONENTS,
    MODIFY_COMPONENTS,
    DELETE_COMPONENTS,
    DELETE_ENTITIES,
    AFTER_ALL,
};
struct Command {
    virtual CommandExecutePriority execute_priority() const noexcept = 0;
    virtual void execute(World &world) const = 0;
    virtual ~Command() {}
};
class CommandSubmit {
    friend class World;

    std::array<std::vector<std::unique_ptr<Command>>,
               (size_t)CommandExecutePriority::AFTER_ALL + 1>
        commands_;

   protected:
    void execute_then_clear(World &world);

   public:
    CommandSubmit() {}

    template <typename Command, typename... Args>
        requires(std::is_constructible_v<Command, Args...>)
    CommandSubmit &submit(Args &&...args) {
        auto cmd = std::make_unique<Command>(std::forward<Args>(args)...);
        commands_[(uint32_t)cmd->execute_priority()].push_back(std::move(cmd));
        return *this;
    }
};
class AttachComponents : public Command {
    Entity entity_;
    std::vector<std::pair<component_t, void *>> components_{};

   public:
    template <typename... Components>
    AttachComponents(Entity entity, Components &&...components):entity_(entity) {
        (add<Components>(std::forward<Components>(components)), ...);
    }

    template <typename Component, typename... Args>
        requires(std::is_constructible_v<Component, Args...>)
    AttachComponents &add(Args &&...args) {
        using Com = purge_t<Component>;
        components_.emplace_back(ComponentIdGenerator<Com>::get(),
                                 new Com(std::forward<Args>(args)...));
        return *this;
    }
    template <typename Component1, typename Component2, typename... Components>
    AttachComponents &add(Component1 &&component1, Component2 &&component2,
                      Components &&...components) {
        add<Component1>(component1);
        add<Component2>(component2);
        (add<Components>(components), ...);
        return *this;
    }
    CommandExecutePriority execute_priority() const noexcept override {
        return CommandExecutePriority::APPEND_COMPONENTS;
    };
    void execute(World &world) const override;
};
}  // namespace ecs
