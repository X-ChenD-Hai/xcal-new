#include "./World.hpp" 
#include "./ComponentAccessor.hpp"
#include "./CommandSubmit.hpp"
#include "/Querier.hpp"
Querier World::queryer() const noexcept { return {*this, {}}; }
ComponentAccessor World::accessor() noexcept { return {*this}; }

CommandSubmit *World::submit() { return &command_submit_; }

World::~World() {}
World::World() {};