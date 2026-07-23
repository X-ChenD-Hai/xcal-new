#include <gtest/gtest.h>

#include <print>

#include "ecs2/comman/dependency_graph.hpp"
#include "ecs2/command.hpp"
#include "ecs2/component.hpp"
#include "ecs2/schedule.hpp"
#include "ecs2/system.hpp"

using namespace xc::ecs;

TEST(Ecs2, Ecs2) {
    Schedule schedule{};
    schedule.registry()
        .regist<int>()
        .regist<double>()
        .regist<float>()
        .regist<long>();
    schedule.add_system<System<ComponentQuery<long>>>(
        [](ComponentQuery<long>& q) { std::println("run_sys"); });
    std::println("{}", schedule.raw_phases());
}

class Msys : public System<CreateEntity<int>, ComponentQuery<int>,
                           DestroyEntity<float>> {};

TEST(Ecs2, Query) {
    using namespace xc::traits;
    using t = type_record<int, type_record<type_record<double>>>;

    using v = flatten_t<t>;
    static_assert(std::is_same_v<v, type_record<int, double>>, "");

    using tt = collect_marker_t<true, ReadWrite, template_record<Read>, t>;
    static_assert(std::is_same_v<tt, ReadWrite<int, double>>, "");
}
TEST(Ecs2, Graphy) {
    DependencyGraph graphy{2};

    graphy.add_edge(0, 1);
    graphy.add_edge(0, 1);

    auto phase = graphy.phases();

    EXPECT_EQ(phase[0][0], 0);
    EXPECT_EQ(phase[1][0], 1);
}