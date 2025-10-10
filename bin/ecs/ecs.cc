#include <IdGenerator.hpp>
#include <Overload.hpp>
#include <SparseList.hpp>
#include <ecs/CommandSubmit.hpp>
#include <ecs/ComponentAccessor.hpp>
#include <ecs/Querier.hpp>
#include <ecs/World.hpp>
#include <ecs/EventBus.hpp>
#include <print>
#include <xc_assert.hpp>

class EntityName {
    std::string name_;
    size_t copy_count_ = 0;

   public:
    EntityName(const std::string &name) : name_(name), copy_count_(0) {}
    ~EntityName() {}
    EntityName(const EntityName &o) : EntityName(o.name_) {
        copy_count_ = o.copy_count_ + 1;
    }
    std::string &name() { return name_; }
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
    AppName(const std::string &name) : name(name) {
        std::println("App name: {}", name);
    }
    ~AppName() { std::println("App name destroyed"); }
};

void update_timer(Timer &timer) {
    std::println(" Current time: {}", timer.time++);
}
void show_name(World &world, Querier querier, ComponentAccessor cmps,
               CommandSubmit &submit, AppName &name, const Timer &timer) {
    if (timer.time == 1) {
        for (size_t i = 0; i < 100; i++) {
            submit.create_entity(EntityName(std::format("Alice {}", i)),
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
        world.quit();
    }
}
class MySystem {
    int count = 11;

   public:
    void system1(const World &) {
        std::println("MySystem::system called {}", count++);
    }
    void system(const World &) {
        std::println("MySystem::system called {}", count++);
    }
    void system(World &, Querier) {
        std::println("MySystem::system called {}", count++);
    }
    void system() const { std::println("MySystem::system called {}", count); }
    void system() { std::println("MySystem::system called {}", count); }
};
void a1(int &) {}
void a1(int &, int &) {}

struct A {
    int a;
    int b;
};
struct B {
    int a;
    size_t b;
};

int main() {
    EventBus event_bus;

    for (size_t i = 0; i < 2; i++) event_bus.publish<char>('a');
    for (size_t i = 0; i < 2; i++) event_bus.publish<A>(i, i + 1);
    for (size_t i = 0; i < 2; i++) event_bus.publish<B>(i, i + 1);
    // auto s = sizeof(uint64_t);

    event_bus.each<A>([](auto &a) { std::println("A: {} , {}", a.a, a.b); });
    std::println("each A");
    event_bus.each<B>([](auto &a) {
        std::println("A: {} , {} and remove", a.a, a.b);
        return true;
    });
    std::println("each A");
    event_bus.each<B>([](auto &a) { std::println("A: {} , {}", a.a, a.b); });
    std::println("clear A");
    event_bus.clear<A>();
    std::println("each A");
    event_bus.each<A>([](auto &a) { std::println("A: {} , {}", a.a, a.b); });
    std::println("insert A");
    for (size_t i = 0; i < 2; i++) event_bus.publish<A>(i, i + 1);
    std::println("each A");
    event_bus.each<A>(
        [](auto &a, auto &bus) { std::println("A: {} , {}", a.a, a.b); },
        event_bus);

    MySystem my_system;
    World world;
    world.add_component<EntityName>()
        ->add_component<EntityUserId>()
        ->add_resource<AppName>("Hello, world!")
        ->add_resource<Timer>(0)
        ->add_system<update_timer>()
        ->add_system<show_name>()
        ->add_system<&MySystem::system1>(&my_system)
        ->add_system<&MySystem::system1>(&my_system)
        ->add_system<Overload<void()>::const_of(&MySystem::system)>(&my_system)
        ->add_system<Overload<void()>::of(&MySystem::system)>(&my_system)
        ->add_system<Overload<void(World &,
        Querier)>::of(&MySystem::system)>(
            &my_system)

        ;

    while (!world.should_quit()) {
        world.update();
    }
    return 0;
}
