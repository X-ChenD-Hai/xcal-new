#pragma once
#include <IdGenerator.hpp>
#include <SparseList.hpp>
#include <functional>
#include <xc_assert.hpp>
#include <print>
#include "./CommandSubmit.hpp"
#include "./ComponentAccessor.hpp"
#include "./ComponentInfo.hpp"
#include "./Entity.hpp"
#include "./Querier.hpp"
#include "./utils/traits.hpp"

using component_t = uint32_t;
using system_t = uint32_t;
class Entity;
class ComponentInfo;
class CommandSubmit;
class Querier;
struct SystemInfo {
    std::vector<size_t> resources_ids_;
    void (*callback_)(World &);
};
template <typename T>
class SystemBuilder {
    World &world_;

   public:
    explicit SystemBuilder(World &world) : world_(world) {}

    World *finish() { return &world_; }
};
template <auto T>
class ObjectSystemBuilder {
    using Self = ObjectSystemBuilder<T>;
    World &world_;
    SystemInfo system_info_;
    bool finished_{false};

   public:
    explicit ObjectSystemBuilder(World &world) : world_(world) {}

    World *build_system();

    template <typename... Resource>
    Self &use_resources();
    World *operator->() { return build_system(); }
    ~ObjectSystemBuilder() { build_system(); }
};

class ResourceIdGenerator {
    static inline size_t next_id_{0};

   public:
    template <typename T>
    static size_t get() {
        static size_t id = _get<purge_t<T>>();
        return id;
    }

   private:
    template <typename T>
    static size_t _get() {
        static size_t id = next_id_++;
        return id;
    }
};

using Cell = std::unique_ptr<void, std::function<void(void *)>>;
struct ResourceInfo {
    size_t id;
    Cell resource_;
};

class World {
    friend class CommandSubmit;
    friend class Querier;
    friend class ComponentAccessor;
    template <typename T>
    friend class SystemBuilder;
    template <auto T>
    friend class ObjectSystemBuilder;

    static constexpr auto Entity2ArchetypeBucktSize = 1024;

   private:
    std::vector<Entity> entities_{};
    std::vector<ComponentInfo> component_infos_{};
    std::vector<ResourceInfo> resource_infos_{};
    std::vector<SystemInfo> system_infos_{};
    SparseList<component_t, uint32_t, 32> component2pool_map_{};
    std::vector<std::vector<Cell>> pools_{};
    CommandSubmit command_submit_;
    bool quit_{false};

   public:
    template <typename Component>
    World *add_component();
    //
    template <typename Resource, typename... Args>
    World *add_resource(Args &&...args);
    template <typename System, typename... Args>
    SystemBuilder<System> add_system(Args... args) {};
    template <auto System>
    ObjectSystemBuilder<System> with_system();
    template <auto System>
    World *add_system();
    template <typename System>
    SystemBuilder<System> add_setup_system() {};
    template <auto System>
    ObjectSystemBuilder<System> add_setup_system() {};

    World();
    ~World();
    bool should_quit() const { return quit_; }
    void quit() { quit_ = true; }
    void start() {quit_ =false;}
    CommandSubmit *submit();
    ComponentAccessor accessor() noexcept;
    Querier queryer() const noexcept;
    void update() {
        std::println("start update__________________");
        for (auto &system_info : system_infos_) {
            system_info.callback_(*this);
        }
        command_submit_.execute(*this);
        std::println("__________________end update");
    }
};
template <auto System>
inline World *World::add_system() {
    using args = func_traits<System>::args_vec;
    using purges = tvector<World, Querier, ComponentAccessor, CommandSubmit>;
    using c_ = tvector<const World, const Querier, const ComponentAccessor,
                       const CommandSubmit>;
    using refs =
        tvector<World &, Querier &, ComponentAccessor &, CommandSubmit &>;
    using c_refs = tvector<const World &, const Querier &,
                           const ComponentAccessor &, const CommandSubmit &>;
    using ppnter = tvector<CommandSubmit *>;
    using real = typename args::template remove_all_from_lists<purges, c_, refs,
                                                               c_refs, ppnter>;
    [this]<typename... Args>(tvector<Args...> *) {
        ObjectSystemBuilder<System>{*this}
            .template use_resources<
                std::remove_pointer_t<std::remove_reference_t<Args>>...>();
    }((real *)0);
    return this;
};

template <typename Resource, typename... Args>
inline World *World::add_resource(Args &&...args) {
    auto idx = resource_infos_.size();
    auto id = ResourceIdGenerator::get<Resource>();
    XC_ASSERT(id == idx);
    resource_infos_.emplace_back(
        idx, Cell((void *)(new Resource(std::forward<Args>(args)...)),
                  [](void *ptr) { delete static_cast<Resource *>(ptr); }));
    return this;
};
template <auto System>
ObjectSystemBuilder<System> World::with_system() {
    return ObjectSystemBuilder<System>{*this};
};

template <typename Component>
inline World *World::add_component() {
    XC_ASSERT(
        !component2pool_map_.has_value(ComponentIdGenerator<Component>::get()));
    auto pool_index = component_infos_.size();
    pools_.emplace_back();
    component_infos_.emplace_back(pool_index, [=](void *ptr) {
        delete (static_cast<purge_t<Component> *>(ptr));
    });
    component2pool_map_.insert(ComponentIdGenerator<Component>::get());
    return this;
}
template <auto T>
template <typename... Resource>
inline typename ObjectSystemBuilder<T>::Self &
ObjectSystemBuilder<T>::use_resources() {
    (
        [&]() {
            auto id = ResourceIdGenerator::get<Resource>();
            XC_ASSERT(id < world_.resource_infos_.size());
            system_info_.resources_ids_.push_back(id);
        }(),
        ...);
    system_info_.callback_ = [](World &world) {
        using args = func_traits<T>::args;
        [&]<typename _F, typename Arg, typename... Args>(
            this auto &&self, _F &&F, std::tuple<Arg, Args...> *) {
            using at = purge_t<Arg>;
            if constexpr (std::is_same_v<at, World>) {
                return self(
                    [F = std::move(F), &world](Args &&...args) {
                        return F(world, std::forward<Args>(args)...);
                    },
                    (std::tuple<Args...> *)nullptr);
            } else if constexpr (std::is_same_v<at, Querier>) {
                return self(
                    [F = std::move(F), &world](Args &&...args) {
                        return F(world.queryer(), std::forward<Args>(args)...);
                    },
                    (std::tuple<Args...> *)nullptr);
            } else if constexpr (std::is_same_v<at, ComponentAccessor>) {
                return self(
                    [F = std::move(F), &world](Args &&...args) {
                        return F(world.accessor(), std::forward<Args>(args)...);
                    },
                    (std::tuple<Args...> *)nullptr);
            } else if constexpr (std::is_same_v<Arg, CommandSubmit *>) {
                return self(
                    [F = std::move(F), &world](Args &&...args) {
                        return F(world.submit(), std::forward<Args>(args)...);
                    },
                    (std::tuple<Args...> *)nullptr);
            } else if constexpr (std::is_same_v<Arg, CommandSubmit &>) {
                return self(
                    [F = std::move(F), &world](Args &&...args) {
                        return F(*world.submit(), std::forward<Args>(args)...);
                    },
                    (std::tuple<Args...> *)nullptr);
            } else {
                return F;
            }
        }(T, (args *)nullptr)(
            *((Resource *)world
                  .resource_infos_[ResourceIdGenerator::get<Resource>()]
                  .resource_.get())...);
    };
    return *this;
}
template <auto T>
inline World *ObjectSystemBuilder<T>::build_system() {
    if (finished_) return &world_;
    finished_ = true;
    world_.system_infos_.emplace_back(system_info_);
    return &world_;
}
