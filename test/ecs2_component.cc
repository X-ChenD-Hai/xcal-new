#include <gtest/gtest.h>

#include <print>
#include <xc/ecs2/ecs.hpp>

#include "ecs2/component.hpp"

using namespace xc::ecs;
TEST(Ecs2, Ecs2) {
    ComponentRegistry registry{};
    EntityFactory enf{};

    auto e1 = enf.spawn();
    auto e2 = enf.spawn();
    registry.regist<int, float, double>();
    registry.insert(e1, 100);
    registry.insert(e1, 100.9f);
    registry.insert(e2, 100.9f);
    registry.insert(e2, 100.9);
    ComponentQuery<float> query{registry};

    auto e = query.query();
    std::println("{}", e);
    query.each([](auto& f) {
        std::println("{}", f);
        f *= 2;
    });
    query.each([](auto& f) { std::println("{}", f); });

    std::println("{}", registry.to_string());
}