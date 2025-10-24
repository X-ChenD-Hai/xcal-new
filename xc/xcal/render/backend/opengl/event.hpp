#pragma once
#include <ecs/command_submit.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/resource_table.hpp>
#include <ecs/world.hpp>
namespace ecs {
class ResourceManager;
class EventBus;
class ResourceTable;
class CommandSubmit;
}  // namespace ecs

namespace xc::xcal::render::opengl {

void handle_event(ecs::ResourceManager& resources, ecs::EventBus& bus,
                  ecs::ResourceTable& table, ecs::CommandSubmit& submit);
}