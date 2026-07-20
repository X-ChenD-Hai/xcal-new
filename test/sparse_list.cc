#include <gtest/gtest.h>

#include <cassert>
#include <format>
#include <print>
#include <xc/ecs2/entity.hpp>

using namespace ecs;

TEST(SparseSet, Basic) {
    SparseSet<Entity> s{};
    EntityFactory factory{};
    auto e1 = factory.spawn();
    auto e2 = factory.spawn();
    auto e3 = factory.spawn();
    auto e4 = factory.spawn();
    auto e5 = factory.spawn();
    s.insert(e1);
    s.insert(e2);
    s.insert(e3);
    s.insert(e4);
    s.insert(e5);
    EXPECT_TRUE(s.contains(e1));
    EXPECT_TRUE(s.contains(e2));
    EXPECT_TRUE(s.contains(e3));
    EXPECT_TRUE(s.contains(e4));
    EXPECT_TRUE(s.contains(e5));
    std::println("{}", s.to_string());
    s.erase(e3);
    factory.free(e3);
    s.erase(e5);
    factory.free(e5);
    auto e13 = factory.spawn();
    auto e15 = factory.spawn();
    s.insert(e13);
    s.insert(e15);
    EXPECT_EQ(s[e13.id()].version(), 1);
    EXPECT_EQ(s[e15.id()].version(), 1);
    EXPECT_TRUE(s.contains(e13));
    EXPECT_TRUE(s.contains(e15));
    EXPECT_FALSE(s.contains(e3));
    EXPECT_FALSE(s.contains(e5));
    std::println("{}", s.to_string());
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
    EXPECT_FALSE(s.contains(e5));
    EXPECT_FALSE(s.contains(e4));
    EXPECT_FALSE(s.contains(e3));
    std::println("{}", s.to_string());
}
