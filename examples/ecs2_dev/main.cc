
#include <exception>
#include <print>
#include <stdexcept>
#include <xc/ecs2/ecs.hpp>

#include "xc/ecs2/component.hpp"
#include "xc/ecs2/schedule.hpp"
#include "xc/ecs2/system.hpp"

using namespace xc::ecs;

class MySys : public System<ComponentQuery<int, ReadWrite<double>>> {
   public:
    using BaseSys::BaseSys;
};
int main() {
    Schedule schedule{};
    schedule.registry()
        .regist<int>()
        .regist<double>()
        .regist<float>()
        .regist<long>();
    schedule.add_system<MySys>();
    std::println("end");
    return 0;
}