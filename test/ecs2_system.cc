#include <gtest/gtest.h>

#include <print>

#include "ecs2/comman/dependency_graph.hpp"
#include "ecs2/comman/traits.hpp"
#include "ecs2/component.hpp"
#include "ecs2/markers.hpp"
#include "ecs2/resource.hpp"
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
}
TEST(Ecs2, Graphy) {
    DependencyGraph graphy{2};

    graphy.add_edge(0, 1);
    graphy.add_edge(0, 1);

    auto phase = graphy.phases();

    EXPECT_EQ(phase[0][0], 0);
    EXPECT_EQ(phase[1][0], 1);
}

TEST(Ecs2, Resouse) {
    namespace tr = xc::traits;
    using t = tr::type_record<Read<int>, Read<double>, ReadWrite<long>,
                              Read<int, float>>;

    using read_t = tr::flatten_t<tr::repack_t<
        tr::filter_if_t<t, tr::not_specialized_from<Read, ReadWrite>>, Read>>;

    // static_assert(std::is_same_v<read_t, Read<int, double,int,float>>, "");
    ResourceRegistry registry;

    registry.create<int>(111);
    registry.create<double>(111);
    registry.create<float>(111);
    EXPECT_EQ(registry.get<int>(), 111);

    ResourceAccessor<int, const double> acc{registry};

    auto& s = acc.get<const int>();
    EXPECT_EQ(s, 111);
    acc.get<int>() = 12;
    EXPECT_EQ(registry.get<int>(), 12);
    auto& v = acc.get<double>();
}