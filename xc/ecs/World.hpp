#pragma once
#include <IdGenerator.hpp>
#include <SparseList.hpp>
#include <functional>
#include <print>
#include <xc_assert.hpp>

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
    void (*callback_)(World &, void *);
    void *callback_obj_{nullptr};
};
template <typename T>
class SystemBuilder {
    World &world_;

   public:
    explicit SystemBuilder(World &world) : world_(world) {}

    World *finish() { return &world_; }
};
template <auto System, typename BindObj = void>
class ObjectSystemBuilder {
    using Self = ObjectSystemBuilder<System, BindObj>;
    World &world_;
    SystemInfo system_info_;
    bool finished_{false};

   public:
    explicit ObjectSystemBuilder(World &world) : world_(world) {}
    explicit ObjectSystemBuilder(World &world, BindObj *obj) : world_(world) {
        system_info_.callback_obj_ = obj;
    }

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
    template <auto System, typename BindObj>
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
    template <auto System, typename BindObj>
    World *add_system(BindObj *obj);
    template <typename System>
    SystemBuilder<System> add_setup_system() {};
    template <auto System>
    ObjectSystemBuilder<System> add_setup_system() {};

    World();
    ~World();
    bool should_quit() const { return quit_; }
    void quit() { quit_ = true; }
    void start() { quit_ = false; }
    CommandSubmit *submit();
    ComponentAccessor accessor() noexcept;
    Querier queryer() const noexcept;
    void update() {
        std::println("start update__________________");
        for (auto &system_info : system_infos_) {
            system_info.callback_(*this, system_info.callback_obj_);
        }
        command_submit_.execute(*this);
        std::println("__________________end update");
    }
};
template <auto System, typename BindObj>
inline World *World::add_system(BindObj *obj) {
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
    [this, obj]<typename... Args>(tvector<Args...> *) {
        ObjectSystemBuilder<System>{*this, obj}
            .template use_resources<
                std::remove_pointer_t<std::remove_reference_t<Args>>...>();
    }((real *)0);
    return this;
};
template <auto System>
inline World *World::add_system() {
    return add_system<System, void>(nullptr);
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

template <auto T, typename BindObj>
inline World *ObjectSystemBuilder<T, BindObj>::build_system() {
    if (finished_) return &world_;
    finished_ = true;
    world_.system_infos_.emplace_back(system_info_);
    return &world_;
}
template <auto T, typename BindObj>
template <typename... Resource>
inline typename ObjectSystemBuilder<T, BindObj>::Self &
ObjectSystemBuilder<T, BindObj>::use_resources() {
    (
        [&]() {
            auto id = ResourceIdGenerator::get<Resource>();
            XC_ASSERT(id < world_.resource_infos_.size());
            system_info_.resources_ids_.push_back(id);
        }(),
        ...);
    system_info_.callback_ = [](World &world, void *obj) {
        using trait = func_traits<T>;
        using args = trait::args;
        auto arghandle = [&]<typename Arg>() -> decltype(auto) {
            using Pt = purge_t<Arg>;
            if constexpr (std::is_same_v<Pt, World>) {
                return world;
            } else if constexpr (std::is_same_v<Arg, Querier>) {
                return world.queryer();
            } else if constexpr (std::is_same_v<Pt, ComponentAccessor>) {
                return world.accessor();
            } else if constexpr (std::is_same_v<Arg, CommandSubmit *>) {
                return world.submit();
            } else if constexpr (std::is_same_v<Arg, CommandSubmit &>) {
                return *world.submit();
            } else {
                return *(
                    (Pt *)world.resource_infos_[ResourceIdGenerator::get<Pt>()]
                        .resource_.get());
            }
        };
        if constexpr (trait::is_member_function) {
            [&]<typename... Args>(std::tuple<Args...> *) -> decltype(auto) {
                (((typename trait::Class *)obj)->*T)(
                    arghandle.template operator()<Args>()...);
            }((args *)nullptr);
        } else {
            [&]<typename... Args>(std::tuple<Args...> *) -> decltype(auto) {
                T(arghandle.template operator()<Args>()...);
            }((args *)nullptr);
        }
    };
    return *this;
}