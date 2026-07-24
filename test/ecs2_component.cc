#include <gtest/gtest.h>

#include <cstddef>
#include <print>
#include <type_traits>

#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/entity.hpp"

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
TEST(Ecs2, Traits) {
    using namespace ::xc::traits;

    using v = type_record<long, int, double>;
    constexpr auto v1 = find_first_v<v, int>;

    using rem = remove_at_t<v, 1>;
    using re = replace_one_t<v, double, float>;

    using bi = batch_insert_t<v, type_record<bind_at<2, float, size_t>>>;

    using is_int = bind_t<std::is_same, bind_at<0, int>>;
    using is_float = bind_t<std::is_same, bind_at<0, float>>;
    constexpr auto a = invoke_meta_v<conjunction<is_float, is_int>, int>;
}