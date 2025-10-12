#include "./World.hpp"

#include "./CommandSubmit.hpp"
#include "./ComponentAccessor.hpp"
#include "/Querier.hpp"

ecs::Querier ecs::World::queryer() const noexcept { return {*this, {}}; }
ecs::ComponentAccessor ecs::World::accessor() noexcept { return {*this}; }

ecs::CommandSubmit &ecs::World::submit() { return *&command_submit_; }

ecs::World::~World() {}
ecs::World::World() {};