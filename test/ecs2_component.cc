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
    registry.insert(e1, 100.91f);
    registry.insert(e2, 100.92f);
    registry.insert(e2, 100.9);
    using Q = ComponentQuery<float, double, ExcludeAny<int>>;
    using Q2 = ComponentQuery<float, ReadWrite<double>, ExcludeAny<int>>;
    Q query{registry};
    Q2 query2{registry};

    auto e = query.query();
    std::println("{}", e);
    query2.each([](auto& f, auto& d) {
        std::println("{},{}", f, d);
        d += f;
    });
    query.each([](auto& f, auto& d) { std::println("{},{}", f, d); });

    std::println("{}", registry);

    std::println("{}", Q::read_component_id_list(registry));
    std::println("{}", Q2::write_component_id_list(registry));
}