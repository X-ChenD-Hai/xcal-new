#include <gtest/gtest.h>

#include <print>
#include <vector>

#include "xc/ecs2/comman/dependency_graph.hpp"
#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/markers.hpp"
#include "xc/ecs2/resource.hpp"
#include "xc/ecs2/schedule.hpp"
#include "xc/ecs2/system.hpp"

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
                           DestroyEntity<float>> {
   public:
    void execute(ComponentRegistry&) override {
        query().each([](auto& o) { std::println("{}", o); });
    }
};
class Msys;
TEST(Ecs2, Query) {
    using base_sys = base_sys_t<CreateEntity<int>, ComponentQuery<int>,
                                DestroyEntity<float>>;

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
    using t = tr::type_record<Read<int>, Read<Read<Read<double>>>,
                              ReadWrite<long>, Read<int, ReadWrite<float>>>;

    using read_t =
        tr::repack_t<tr::filter_if_t<t, tr::is_specialized_from<Read>>, Read>;

    using t1 = Read<Read<std::vector<int>>>;
    using t2 = xc::traits::flatten_t<t1>;

    using vr1 = tr::value_record<1, 3, 5>;
    using vr2 = tr::value_record<4, 5, 6, 7, 8>;
    using vr3 = tr::value_record<>;
    using vr4 = tr::value_record<2, 1, 3, 4, 23, 455, 5>;
    using k1 = tr::as_integral_type_record_t<vr1>;
    using k2 = tr::as_integral_type_record_t<vr2>;
    using k3 = tr::as_integral_type_record_t<vr3>;
    using k4 = tr::as_integral_type_record_t<vr4>;
    using c1 = tr::order_preserving_merge_t<k1, k2>;
    using cc1 = tr::as_value_record_t<c1>;
    static_assert(
        std::is_same_v<xc::traits::value_record<1, 3, 4, 5, 5, 6, 7, 8>, cc1>);
    using sss1 = tr::sort_t<vr4>;
    static_assert(
        std::is_same_v<xc::traits::value_record<1, 2, 3, 4, 5, 23, 455>, sss1>);
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