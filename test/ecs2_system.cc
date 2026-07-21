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
