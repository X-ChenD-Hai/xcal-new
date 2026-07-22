#include <gtest/gtest.h>

#include <print>
#include <xc/ecs2/ecs.hpp>

#include "ecs2/component.hpp"
#include "ecs2/entity.hpp"

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
    ComponentQueryCachePool pool{registry};
    using Q2 = ComponentQuery<float, ReadWrite<double>, ExcludeAny<int>>;
    auto query = pool.query<float, double, ExcludeAny<int>>();
    using Q = decltype(query);
    auto query2 = pool.query<Q2>();

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

TEST(Ecs2, EntityFactory) {
    EntityFactory f{};

    auto e = f.spawn();
    std::println("{}", e);
    f.free(e);
    auto e2 = f.spawn();
    std::println("{}", e2);
    EXPECT_EQ(e2.id(), e.id());
}