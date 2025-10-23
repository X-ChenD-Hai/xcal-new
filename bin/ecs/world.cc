#include <ecs/command_submit.hpp>
#include <ecs/component_accessor.hpp>
#include <ecs/querier.hpp>
#include <ecs/world.hpp>
#include <id_generator.hpp>
#include <print>
#include <sparse_list.hpp>
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

int main() {
    ecs::World world;
    world.add_component<EntityName>().add_component<EntityUserId>();
    EntityName name{"data"};
    world.submit()
        .submit<ecs::AttachComponents>(name)
        .submit<ecs::AttachComponents>(name, EntityUserId{11})
        .submit<ecs::AttachComponents>(EntityUserId{22});
    auto q1 = world.queryer().query<EntityName>();
    auto q2 = world.queryer().query<EntityUserId>();
    auto q3 = q1.query<EntityUserId>();
    auto ac = world.accessor();

    std::println("e1 :{}", q1.entities().size());
    for (auto e : q1.entities()) {
        std::println("e :{}", e.id());
    }
    std::println("e2 :{}", q2.entities().size());
    for (auto e : q2.entities()) {
        std::println("e :{}", e.id());
    }
    std::println("e3 :{}", q3.entities().size());
    for (auto e : q3.entities()) {
        std::println("e :{}", e.id());
    }

    for (auto e : q1.entities()) {
        std::println("query1: name {}", ac.data<EntityName>(e)->name());
    }
    for (auto e : q2.entities()) {
        std::println("query2: id {}", ac.data<EntityUserId>(e)->id);
    }
    for (auto e : q3.entities()) {
        std::println("query2: name {}", ac.data<EntityName>(e)->name());
        std::println("query3: id {}", ac.data<EntityUserId>(e)->id);
    }
    return 0;
}