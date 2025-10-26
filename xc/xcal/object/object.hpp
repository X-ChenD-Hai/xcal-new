#pragma once
#include <ecs/entity.hpp>
#include <ecs/event_bus.hpp>
#include <ecs/world.hpp>

#include "../xcal.hpp"
#include "xc_assert.hpp"

namespace xc::xcal {
class Xcal;
namespace animation {
class Animation;
}
}  // namespace xc::xcal
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
    friend class xcal::animation::Animation;
    ecs::Entity entity_{0, 0};
    ecs::World *world_{nullptr};

   public:
    // 访问当前对象绑定的实体（只读）
    inline ecs::Entity entity() const noexcept { return entity_; }
    // 访问 World（断言已绑定）
    inline ecs::World &world() const {
        XC_ASSERT(world_ != nullptr);
        return *world_;
    }

   protected:
    Object(const Object &) = default;
    Object &operator=(const Object &) {
        XC_ASSERT(entity_.id() == 0);
        return *this;
    };

   public:
    template <typename Component>
    inline bool has_component() const noexcept {
        return world().component_info<Component>().has_entity(entity_);
    }
    template <typename Component>
    inline Component &component() {
        return *world().get_or_attach_component<Component>(entity_);
    }
    template <typename Component>
    inline const Component &component() const {
        return world().component<Component>(entity_);
    }
    template <typename Component>
    inline Component &set_component(const Component &component) {
        return *world().get_or_attach_component<Component>(entity_) = component;
    }

   public:
    Object() = default;
    Object(Object &&) = default;
    Object &operator=(Object &&) = default;
    virtual ~Object() = 0;
};
inline Object::~Object() = default;
}  // namespace xc::xcal::object
template <typename T, typename... Args>
    requires std::derived_from<T, xc::xcal::object::Object>
inline T xc::xcal::Xcal::add(Args &&...args) {
    auto obj = T(std::forward<Args>(args)...);
    setup_object(obj);
    world_.resource<ecs::EventBus>().publish<object::CreateObject<T>>(
        obj.entity_, obj.config_);
    return obj;
}
template <typename T>
    requires std::derived_from<T, xc::xcal::object::Object>
inline T xc::xcal::Xcal::add(T &&obj_ref) {
    setup_object(obj_ref);
    world_.resource<ecs::EventBus>().publish<object::CreateObject<T>>(
        obj_ref.entity_, obj_ref.config_);
    return obj_ref;
}