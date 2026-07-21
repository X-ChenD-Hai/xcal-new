#include <gtest/gtest.h>

#include <print>
#include <xc/ecs2/ecs.hpp>

#include "ecs2/component.hpp"
#include "ecs2/schedule.hpp"
#include "ecs2/system.hpp"

using namespace xc::ecs;

class MySys : public System<ComponentQuery<int, ReadWrite<double>>> {
   public:
    using BaseSys::BaseSys;
};

TEST(Ecs2, Ecs2) {
    Schedule schedule{};
    schedule.registry()
        .regist<int>()
        .regist<double>()
        .regist<float>()
        .regist<long>();
    schedule.add_system<System<ComponentQuery<int, ReadWrite<double>>>>()
        .add_system<System<ComponentQuery<double, ReadWrite<long>>>>()
        .add_system<System<ComponentQuery<float>>>()
        .add_system<System<ComponentQuery<long>>>();
    std::println("{}", schedule.raw_phases());
}
TEST(Ecs2, Query) {
    using namespace details;
    using t = type_record<int, double>;

    using tt = collect_marker_t<false, ReadWrite, template_record<Read>, t>;
}
