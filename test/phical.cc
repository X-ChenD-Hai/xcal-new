#include <gtest/gtest.h>

#include <ecs/world.hpp>

struct Posion {
    float x;
    float y;
};

struct Speed {
    float dx;
    float dy;
};

TEST(Phical, phical) {
    using namespace ecs;

    World world;

    auto e = world.create_entity();

    world.add_component<Posion>().add_component<Speed>();
}
