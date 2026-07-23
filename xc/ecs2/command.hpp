#pragma once
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/entity.hpp"
#include "xc/ecs2/markers.hpp"

namespace xc::ecs {

template <typename U>
using component_t = std::conditional_t<traits::is_specialized_v<Optional, U>,
                                       std::optional<typename U::type>, U>;
template <typename... T>
class CommandQueue;
template <typename... T>
class CommandQueue<CreateEntity<T...>> {
   public:
    void create(const component_t<T>&... components) {
        components_.push_back(std::make_tuple(components...));
    }
    const std::vector<std::tuple<T...>>& components() const {
        return components_;
    }
    void clear_create_command() { components_.clear(); }

   private:
    std::vector<std::tuple<component_t<T>...>> components_;
};
template <typename... T>
class CommandQueue<DestroyEntity<T...>> {
   public:
    void destroy(const Entity& entity) { entities_.push_back(entity); }
    void destroy(const std::vector<Entity>& entities) {
        entities_.insert(entities_.end(), entities.begin(), entities.end());
    }
    const std::vector<Entity>& entities() const { return entities_; }
    void clear_destroy_command() { entities_.clear(); }

   private:
    std::vector<Entity> entities_;
};
template <typename... T>
class CommandQueue<Attach<T...>> {
   public:
    struct AttachSlot {
        Entity entity;
        std::tuple<component_t<T>...> components;
    };
    void attach(const Entity& entity, component_t<T>&... components) {
        attach_slots_.push_back({entity, std::make_tuple(components...)});
    }
    void clear_attach_command() { attach_slots_.clear(); }
    const std::vector<AttachSlot>& attach_slots() const {
        return attach_slots_;
    }

   private:
    std::vector<AttachSlot> attach_slots_;
};
template <typename... T>
class CommandQueue<Detach<T...>> {
   public:
    void detach(const Entity& entity, const T&... components) {
        detach_entities_.push_back(entity);
    }
    void clear_detach_command() { detach_entities_.clear(); }

   private:
    std::vector<Entity> detach_entities_;
};
}  // namespace xc::ecs