#include <gtest/gtest.h>

#include <print>
#include <xc/ecs2/ecs.hpp>

#include "ecs2/component.hpp"
#include "ecs2/schedule.hpp"
#include "ecs2/system.hpp"

using namespace xc::ecs;

// class MySys : public System<ComponentQuery<int, ReadWrite<double>>> {
//    public:
//     using BaseSys::BaseSys;
// };

// TEST(Ecs2, Ecs2) {
//     Schedule schedule{};
//     schedule.registry()
//         .regist<int>()
//         .regist<double>()
//         .regist<float>()
//         .regist<long>();
//     schedule.add_system<System<ComponentQuery<int, ReadWrite<double>>>>()
//         .add_system<System<ComponentQuery<double, ReadWrite<long>>>>()
//         .add_system<System<ComponentQuery<float>>>()
//         .add_system<System<ComponentQuery<long>>>(
//             [](ComponentQuery<long>& q) { std::println("run_sys"); });
//     std::println("{}", schedule.raw_phases());
// }
TEST(Ecs2, Query) {
    using namespace xc::traits;
    using t = type_record<int, type_record<type_record<double>>>;

    using v = flatten_t<t>;
    static_assert(std::is_same_v<v, type_record<int, double>>, "");

    using tt = collect_marker_t<true, ReadWrite, template_record<Read>, t>;
    static_assert(std::is_same_v<tt, ReadWrite<int, double>>, "");
}
