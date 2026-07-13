#pragma once
#include <xc/ecs/command_submit.hpp>
#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/resource_table.hpp>
#include <xc/ecs/world.hpp>
#include <xc/ecs/types.hpp>
namespace ecs {
class ResourceManager;
class ResourceTable;
class CommandSubmit;
}  // namespace ecs

namespace xc::xcal::render::opengl {

void handle_event(ecs::ResourceManager& resources, ecs::EventBus& bus,
                  ecs::ResourceTable& table, ecs::CommandSubmit& submit);
}