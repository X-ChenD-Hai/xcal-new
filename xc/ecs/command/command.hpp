#pragma once
#include <cstdint>
namespace ecs {
class World;
namespace command {
enum class ExecutePriority : uint16_t {
    BEFORE_ALL = 0,
    APPEND_COMPONENTS,
    MODIFY_COMPONENTS,
    DELETE_COMPONENTS,
    DELETE_ENTITIES,
    AFTER_ALL,
};

struct Command {
    virtual ExecutePriority execute_priority() const noexcept = 0;
    virtual void execute(World& world) const = 0;
    virtual ~Command() {}
};
}  // namespace command
}  // namespace ecs
