#pragma once
#include <ecs/EventBus.hpp>
#include <ecs/Resource.hpp>
#include <ecs/ResourceTable.hpp>

namespace xc::xcal::render::opengl {

void handle_event(ecs::ResourceManager& resources, ecs::EventBus& bus,
                  ecs::ResourceTable& table);
}