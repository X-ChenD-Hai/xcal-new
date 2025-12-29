#include <chrono>
#include <ecs/command/attach_components.hpp>
#include <ecs/command_submit.hpp>
#include <ecs/component_accessor.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/querier.hpp>
#include <ecs/resource_table.hpp>
#include <ecs/world.hpp>
#include <id_generator.hpp>
#include <overload.hpp>
#include <print>
#include <sparse_list.hpp>
#include <xc_assert.hpp>
using namespace ecs;
class EntityName {
    std::string name_;
    size_t copy_count_ = 0;

   public:
    EntityName(const std::string& name) : name_(name), copy_count_(0) {}
    ~EntityName() {}
    EntityName(const EntityName& o) : EntityName(o.name_) {
        copy_count_ = o.copy_count_ + 1;
    }
    std::string& name() { return name_; }
};
struct EntityUserId {
    size_t id;
};

struct Timer {
    size_t time;
    Timer(size_t time) : time(time) { std::println("Timer created: {}", time); }
    ~Timer() { std::println("Timer destroyed"); }
};
struct AppName {
    std::string name;
    AppName(const std::string& name) : name(name) {
        std::println("App name: {}", name);
    }
    ~AppName() { std::println("App name destroyed"); }
};
struct Quit {};
void update_timer(Timer& timer) {
    std::println(" Current time: {}", timer.time++);
}
void show_name(World& world, Querier querier, ComponentAccessor cmps,
               CommandSubmit& submit, EventBus& bus, AppName& name,
               const Timer& timer) {
    if (timer.time == 1) {
        for (size_t i = 0; i < 100; i++) {
            submit.submit<ecs::command::AttachComponents>(
                world.create_entity(), EntityName(std::format("Alice {}", i)),
                EntityUserId{1});
        }
    }
    if (timer.time >= 5) {
        for (auto entity :
             querier.query<EntityName, EntityUserId>().entities()) {
            std::println("entity {} destroyed", entity.id());
            if (auto name = cmps.data<EntityName>(entity); name) {
                std::println("{}", name->name());
            }
        }
        bus.publish<Quit>();
    }
}
class MySystem {
    int count = 11;

   public:
    void system1(const World&) {
        std::println("MySystem::system called {}", count++);
    }
    void system(const World&) {
        std::println("MySystem::system called {}", count++);
    }
    void system(World&, Querier) {
        std::println("MySystem::system called {}", count++);
    }
    void system() const { std::println("MySystem::system called {}", count); }
    void system() { std::println("MySystem::system called {}", count); }
};
void a1(int&) {}
void a1(int&, int&) {}

struct A {
    int a;
    int b;
};
struct B {
    int a;
    size_t b;
};
struct ReadyToExit {};

class MyResource {
   public:
    int a = 10;
    int b = 20;

    MyResource(int a, int b) : a(a), b(b) {
        std::println("MyResource created with a = {}, b = {}", a, b);
    }

    ~MyResource() {
        std::println("MyResource destroyed with a = {}, b = {}", a, b);
    }

    class Dynamic {};
    class Static {};
};
static constexpr size_t LOOP_COUNT = 100;
void read_resource(ResourceTable& table, EventBus& bus, Timer& timer) {
    if (bus.exist<ReadyToExit>()) {
        table.async_release_resource<MyResource>();
        table.async_release_resource<MyResource, int>();
        return;
    }

    auto res = table.get_resource<MyResource>();
    auto res2 = table.get_resource<MyResource, int>();
    if (!res || !res2) {
        table.async_create_or_get<MyResource>(1, 1);
        table.async_create_or_get<MyResource, int>(1, 1);
        return;
    }
    std::println("MyResource: id = {},  a = {}, b = {}",
                 table.resource_id<MyResource>(), res->a++, res->b++);
    std::println("MyResource: res2 id = {},  a = {}, b = {}",
                 table.resource_id<MyResource, int>(), res2->a++, res2->b++);
}
void update_epoch(World& world, EventBus& bus, Timer& timer) {
    timer.time++;
    std::println("Current epoch: {}", timer.time);
    if (timer.time == LOOP_COUNT - 1) {
        bus.publish<ReadyToExit>();
    } else if (timer.time == LOOP_COUNT) {
        bus.clear<ReadyToExit>();
        bus.publish<Quit>();
    }
}
void do_async_create_resource(ResourceTable& table) {
    std::println("do_async_create_resource called");
    table.do_async_create_tasks();
}

int main(int argc, char* argv[]) {
    MySystem my_system;
    World world;
    world.regist_component<EntityName>()
        .regist_component<EntityUserId>()
        .add_resource<AppName>("Hello, world!")
        .add_resource<ResourceTable>()
        .add_resource<EventBus>()
        .add_resource<Timer>(0);
    ;
    auto start = std::chrono::high_resolution_clock::now();
    while (!world.resource<EventBus>().exist<Quit>()) {
        world.resource<EventBus>().clear();
        world.run_system<update_epoch>()
            .run_system<read_resource>()
            .run_system<do_async_create_resource>();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::println("loop: {:}",
                 std::chrono::duration_cast<std::chrono::duration<double>>(
                     (end - start)));
    std::println(
        "loop avg: {:}",
        std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
            (end - start)) /
            LOOP_COUNT);
    return 0;
}
