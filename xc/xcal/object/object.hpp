#pragma once
#include <ecs/entity.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/world.hpp>

#include "../xcal.hpp"

namespace xc::xcal {
class Xcal;
}
namespace xc::xcal::object {
template <typename T>
struct CreateObject {
    ecs::Entity entity;
    T::Config config;
};
template <typename T>
struct CloneObject {
    ecs::Entity entity;
};
template <typename T>
struct DestroyObject {
    ecs::Entity entity;
};
class Object {
    friend class xcal::Xcal;
    ecs::Entity entity_{0, 0};

   public:
    Object() = default;
    Object(const Object &) { XC_ASSERT(entity_.id() == 0); };
    Object &operator=(const Object &) {
        XC_ASSERT(entity_.id() == 0);
        return *this;
    };
    Object(Object &&) = default;
    Object &operator=(Object &&) = default;
    virtual ~Object() = 0;
};
inline Object::~Object() = default;
}  // namespace xc::xcal::object
template <typename T, typename... Args>
    requires std::derived_from<T, xc::xcal::object::Object>
inline T &xc::xcal::Xcal::add(Args &&...args) {
    auto &obj = static_cast<T &>(
        add_object(std::make_unique<T>(std::forward<Args>(args)...)));
    world_.resource<ecs::EventBus>().publish<object::CreateObject<T>>(
        obj.entity_, obj.config_);
    return obj;
}
template <typename T>
    requires std::derived_from<T, xc::xcal::object::Object>
inline T &xc::xcal::Xcal::add(T &&obj_ref) {
    auto &obj = static_cast<T &>(
        add_object(std::make_unique<T>(std::move(obj_ref))));
    world_.resource<ecs::EventBus>().publish<object::CreateObject<T>>(
        obj.entity_, obj.config_);
    return obj;
}