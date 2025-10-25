#include "./xcal.hpp"

#include <ecs/event_bus.hpp>
#include <ecs/world.hpp>

#include "./animation/animation.hpp"
#include "./camera/camera.hpp"
#include "./object/object.hpp"
#include "./transform/transform.hpp"

void xc::xcal::handle_event(ecs::EventBus &bus) {}

xc::xcal::Xcal *xc::xcal::Xcal::install(ecs::World &world) {
    if (!world.resource_manager().has<ecs::EventBus>())
        world.add_resource<ecs::EventBus>();
    camera::details::setup(world);
    transform::details::setup(world);
    animation::details::setup(world);
    return new Xcal(world);
}
ecs::World &xc::xcal::Xcal::run(ecs::World &world) {
    return world.run_system<handle_event>()
        .run_system<animation::details::run>()
        .run_system<camera::details::run>()
        .run_system<transform::details::run>();
}
void xc::xcal::Xcal::uninstall(ecs::World &world, Xcal *xcal) { delete xcal; };
xc::xcal::object::Object &xc::xcal::Xcal::add_object(
    std::unique_ptr<object::Object> &&obj) {
    obj->entity_ = world_.create_entity();
    obj->world_ = &world_;  // 绑定 world 指针，便于对象内部访问与提交命令
    objects_.emplace_back(std::move(obj));
    return *objects_.back();
}
xc::xcal::Xcal::Xcal(ecs::World &world) : world_(world) {}
