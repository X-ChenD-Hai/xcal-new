#pragma once
#include <ecs/event_bus.hpp>
#include <ecs/resource.hpp>
#include <ecs/resource_table.hpp>

namespace xc::xcal::render::opengl {

void handle_event(ecs::ResourceManager& resources, ecs::EventBus& bus,
                  ecs::ResourceTable& table);
}