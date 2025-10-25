#pragma once
#include <memory>
#include <vector>
namespace ecs {
class World;
class EventBus;
};  // namespace ecs

namespace xc::xcal {
namespace animation {
class AnimationManager;
class TimeLine;
}  // namespace animation
namespace object {
class Object;
}
class Xcal {
    friend class ecs::World;
    ecs::World &world_;
    std::vector<std::unique_ptr<xc::xcal::object::Object>> objects_;

   protected:
    Xcal(ecs::World &world);
    Xcal(const Xcal &) = delete;
    Xcal &operator=(const Xcal &) = delete;
    Xcal(Xcal &&) = delete;
    Xcal &operator=(Xcal &&) = delete;
    ~Xcal() = default;
    static Xcal *install(ecs::World &world);
    static void uninstall(ecs::World &world, Xcal *xcal);
    ecs::World &run(ecs::World &world);

   private:
    object::Object &add_object(std::unique_ptr<object::Object> &&obj);

   public:
    template <typename T, typename... Args>
        requires std::derived_from<T, xc::xcal::object::Object>
    T &add(Args &&...args);
    template <typename T>
        requires std::derived_from<T, xc::xcal::object::Object>
    T &add(T &&obj);
    animation::AnimationManager &animation_manager();
};
void handle_event(ecs::EventBus &bus);
}  // namespace xc::xcal
