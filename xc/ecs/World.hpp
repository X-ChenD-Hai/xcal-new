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
#include "./Resource.hpp"
#include "./utils/traits.hpp"
namespace ecs {

using component_t = uint32_t;
using system_t = uint32_t;
class Entity;
class ComponentInfo;
class CommandSubmit;
class Querier;

template <auto System, typename BindObj = void>
class ObjectSystemBuilder;
struct SystemInfo;

struct SystemInfo {
    std::vector<size_t> resources_ids_;
    void (*callback_)(World &, void *);
    void *callback_obj_{nullptr};
};
template <auto System, typename BindObj>
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

   protected:
    template <class Arg>
    static decltype(auto) fatch_args(World &world) noexcept;

   public:
    template <typename Component>
    World *add_component();
    //
    template <typename Resource, typename... Args>
    World *add_resource(Args &&...args);
    template <auto System, typename BindObj>
    ObjectSystemBuilder<System, BindObj> with_system();
    template <auto System>
    World *add_system();
    template <auto System, typename BindObj>
    World *add_system(BindObj *obj);
    template <auto System>
    World *run_system();
    template <auto System, typename BindObj>
    World *run_system(BindObj *obj);
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
        execute_commands();
        std::println("__________________end update");
    }
    void execute_commands() { command_submit_.execute(*this); }
    void execute_commands(CommandSubmit *submit) { submit->execute(*this); }
};
template <auto System, typename BindObj>
inline World *World::run_system(BindObj *obj) {
    using trait = func_traits<System>;
    using args = trait::args;
    static_assert(std::derived_from<BindObj, typename trait::Class>,
                  "not support bind obj");
    []<typename... Args>(std::tuple<Args...> *, World &world,
                         void *obj) -> decltype(auto) {
        (((typename trait::Class *)obj)->*System)(
            World::fatch_args<Args>(world)...);
    }((args *)nullptr, *this, obj);
    return this;
};
template <auto System>
inline World *World::run_system() {
    using trait = func_traits<System>;
    using args = trait::args;
    []<typename... Args>(std::tuple<Args...> *,
                         World &world) -> decltype(auto) {
        System(World::fatch_args<Args>(world)...);
    }((args *)nullptr, *this);
    return this;
}

template <auto System, typename BindObj>
World *World::add_system(BindObj *obj) {
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
        ObjectSystemBuilder<System, BindObj>{*this, obj}
            .template use_resources<
                std::remove_pointer_t<std::remove_reference_t<Args>>...>();
    }((real *)0);
    return this;
};
template <auto System>
World *World::add_system() {
    return add_system<System, void>(nullptr);
};

template <typename Resource, typename... Args>
World *World::add_resource(Args &&...args) {
    auto idx = resource_infos_.size();
    auto id = ResourceIdGenerator::get<Resource>();
    XC_ASSERT(id == idx);
    resource_infos_.emplace_back(
        idx, Cell((void *)(new Resource(std::forward<Args>(args)...)),
                  [](void *ptr) { delete static_cast<Resource *>(ptr); }));
    return this;
};

template <typename Component>
World *World::add_component() {
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
template <auto System, typename BindObj>
ObjectSystemBuilder<System, BindObj> World::with_system() {
    return ObjectSystemBuilder<System>{*this};
};

template <class Arg>
decltype(auto) World::fatch_args(World &world) noexcept {
    using Pt = purge_t<Arg>;
    using Np = std::remove_cv_t<Arg>;
    if constexpr (std::is_same_v<Np, World>) {
        static_assert(false, "not support world");
    } else if constexpr (std::is_same_v<Np, World &>) {
        return std::ref(world);
    } else if constexpr (std::is_same_v<Arg, const World &>) {
        return std::cref(world);
    } else if constexpr (std::is_same_v<Np, World *>) {
        return &world;
    } else if constexpr (std::is_same_v<Np, Querier>) {
        return world.queryer();
    } else if constexpr (std::is_same_v<Np, Querier &> ||
                         std::is_same_v<Np, Querier *>) {
        static_assert(false, "not support Querier ref or ptr");
    } else if constexpr (std::is_same_v<Np, ComponentAccessor>) {
        return world.accessor();
    } else if constexpr (std::is_same_v<Np, ComponentAccessor &> ||
                         std::is_same_v<Np, ComponentAccessor *>) {
        static_assert(false, "not support ComponentAccessor ref or ptr");
    } else if constexpr (std::is_same_v<Np, CommandSubmit *>) {
        return world.submit();
    } else if constexpr (std::is_same_v<Np, CommandSubmit &>) {
        return *world.submit();
    } else if constexpr (std::is_same_v<Np, CommandSubmit>) {
        static_assert(false, "use CommandSubmit by ref or ptr");
    } else if constexpr (std::is_same_v<Np, Pt &>) {
        return std::ref(
            *((Pt *)world.resource_infos_[ResourceIdGenerator::get<Pt>()]
                  .resource_.get()));
    } else if constexpr (std::is_same_v<Np, const Pt &>) {
        return std::cref(
            *((Pt *)world.resource_infos_[ResourceIdGenerator::get<Pt>()]
                  .resource_.get()));
    } else if constexpr (std::is_same_v<Np, const Pt *> ||
                         std::is_same_v<Np, const Pt *>) {
        return (const Pt *)world.resource_infos_[ResourceIdGenerator::get<Pt>()]
            .resource_.get();
    } else if constexpr (std::is_same_v<Np, Pt *> ||
                         std::is_same_v<Np, const Pt *>) {
        return (Pt *)world.resource_infos_[ResourceIdGenerator::get<Pt>()]
            .resource_.get();
    } else {
        static_assert(false, "not support this type");
    }
}
template <auto System, typename BindObj>
template <typename... Resource>
typename ObjectSystemBuilder<System, BindObj>::Self &
ObjectSystemBuilder<System, BindObj>::use_resources() {
    (
        [&]() {
            auto id = ResourceIdGenerator::get<Resource>();
            XC_ASSERT(id < world_.resource_infos_.size());
            system_info_.resources_ids_.push_back(id);
        }(),
        ...);
    if constexpr (std::is_member_function_pointer_v<decltype(System)>)
        system_info_.callback_ = [](World &world, void *obj) {
            world.run_system<System, BindObj>(static_cast<BindObj *>(obj));
        };
    else
        system_info_.callback_ = [](World &world, void *obj) {
            world.run_system<System>();
        };
    return *this;
}
template <auto T, typename BindObj>
World *ObjectSystemBuilder<T, BindObj>::build_system() {
    if (finished_) return &world_;
    finished_ = true;
    world_.system_infos_.emplace_back(system_info_);
    return &world_;
}
}  // namespace ecs
