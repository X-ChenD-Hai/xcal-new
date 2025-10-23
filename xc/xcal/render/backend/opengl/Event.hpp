#pragma once
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