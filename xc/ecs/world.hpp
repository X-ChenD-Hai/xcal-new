#pragma once
#include <functional>
#include <id_generator.hpp>
#include <print>
#include <sparse_list.hpp>
#include <type_map.hpp>
#include <xc_assert.hpp>

#include "./command_submit.hpp"
#include "./component_accessor.hpp"
#include "./component_info.hpp"
#include "./entity.hpp"
#include "./querier.hpp"
#include "./resource.hpp"
#include "./utils/traits.hpp"

namespace ecs {

using component_t = uint32_t;
using system_t = uint32_t;
class Entity;
class ComponentInfo;
class CommandSubmit;
class Querier;

class World {
    friend class CommandSubmit;
    friend class Querier;
    friend class ComponentAccessor;
    template <typename T>
    friend class SystemBuilder;
    template <auto System, typename BindObj>
    friend class ObjectSystemBuilder;

   private:
    std::vector<Entity> entities_{};
    std::vector<ComponentInfo> component_infos_{};
    ResourceManager resource_manager_;
    SparseList<component_t, uint32_t, 32> component2pool_map_{};
    std::vector<std::vector<Cell_>> pools_{};
    CommandSubmit command_submit_;
    std::vector<std::unique_ptr<void, std::function<void(void *)>>> plugins_;
    TypeMap<uint32_t> plugins_id_map_{uint32_t(-1)};

   protected:
    template <class Arg>
    static decltype(auto) fatch_args(World &world) noexcept;

   public:
    template <typename Component>
    World &add_component();
    template <typename Component1, typename Component2, typename... Components>
    World &add_component() {
        add_component<Component1>();
        add_component<Component2>();
        (add_component<Components>(), ...);
        return *this;
    }
    template <typename Resource>
    World &add_resource(Resource *resource);
    ResourceManager &resource_manager() { return resource_manager_; }
    template <typename Resource>
    Resource &resource();
    template <typename Resource, typename... Args>
    World &add_resource(Args &&...args);
    template <auto System>
    World &run_system();
    template <auto System, typename BindObj>
    World &run_system(BindObj *obj);
    template <typename Plugin, typename... Args>
    World &use_plugin(Args &&...args) {
        std::println("installing plugin: {}", typeid(Plugin).name());
        if constexpr (std::is_invocable_r_v<Plugin *, decltype(Plugin::install),
                                            World &, Args...>) {
            plugins_.emplace_back(
                Plugin::install(*this, std::forward<Args>(args)...),
                [this](void *ptr) { Plugin::uninstall(*this, (Plugin *)ptr); });
            plugins_id_map_.data<Plugin>() = plugins_.size() - 1;
            std::println("installed plugin: {} , id:{}", typeid(Plugin).name(),
                         plugins_id_map_.data<Plugin>());

        } else {
            Plugin::install(*this, std::forward<Args>(args)...);
            std::println("installed plugin: {}", typeid(Plugin).name());
        }
        return *this;
    }
    template <typename Plugin>
    inline constexpr World &run_plugin() {
        if constexpr (std::is_member_function_pointer_v<
                          decltype(&Plugin::run)>) {
            XC_ASSERT(plugins_id_map_.data<Plugin>() != uint32_t(-1));
            (static_cast<Plugin *>(
                 plugins_[plugins_id_map_.data<Plugin>()].get()))
                ->run(*this);
        } else {
            Plugin::run(*this);
        }
        return *this;
    }
    template <typename Plugin1, typename Plugin2, typename... Plugins>
    inline constexpr World &run_plugin() {
        run_plugin<Plugin1>();
        run_plugin<Plugin2>();
        (run_plugin<Plugins>(), ...);
        return *this;
    }
    template <typename Plugin>
    Plugin &plugin() {
        auto index = plugins_id_map_.data<Plugin>();
        XC_ASSERT(index != uint32_t(-1));
        return *(static_cast<Plugin *>(plugins_[index].get()));
    }

    World();
    ~World();
    inline Entity create_entity() {
        return entities_.emplace_back(entities_.size() + 1, 0);
    }
    inline bool has_component(component_t comp) const noexcept {
        return component2pool_map_.has_value(comp);
    }
    inline size_t pool_index_of_component(component_t comp) const noexcept {
        return component2pool_map_.get_index(comp);
    }
    inline const ComponentInfo &component_info(
        component_t comp) const noexcept {
        XC_ASSERT(has_component(comp));
        return component_infos_[pool_index_of_component(comp)];
    }
    inline ComponentInfo &component_info(component_t comp) noexcept {
        XC_ASSERT(has_component(comp));
        return component_infos_[pool_index_of_component(comp)];
    }
    inline void attach_component(component_t comp, Entity e, void *data) {
        auto &info = component_info(comp);
        XC_ASSERT(!info.has_entity(e));
        auto pool_index = info.pool_index();
        pools_[pool_index].emplace_back(data, info.deleter_);
        info.add_entity(e);
    }
    inline void detach_component(component_t comp, Entity e) {
        auto &info = component_info(comp);
        XC_ASSERT(info.has_entity(e));
        pools_[info.pool_index()][info.cell_index(e)].reset();
        info.remove_entity(e);
    }
    inline void modify_component(component_t comp, Entity e, void *data) {
        auto &info = component_info(comp);
        XC_ASSERT(info.has_entity(e));
        pools_[info.pool_index()][info.cell_index(e)] =
            Cell_(data, info.deleter_);
    }

    CommandSubmit &submit();
    ComponentAccessor accessor() noexcept;
    Querier queryer() const noexcept;
    World &execute_commands() {
        command_submit_.execute_then_clear(*this);
        return *this;
    }
    World &execute_commands(CommandSubmit *submit) {
        submit->execute_then_clear(*this);
        return *this;
    }
};
template <typename Resource>
inline Resource &World::resource() {
    XC_ASSERT(resource_manager_.has<Resource>());
    return resource_manager_.get<Resource>();
};
template <auto System, typename BindObj>
inline World &World::run_system(BindObj *obj) {
    using trait = func_traits<System>;
    using args = trait::args;
    static_assert(std::derived_from<BindObj, typename trait::Class>,
                  "not support bind obj");
    []<typename... Args>(std::tuple<Args...> *, World &world,
                         void *obj) -> decltype(auto) {
        (((typename trait::Class *)obj)->*System)(
            World::fatch_args<Args>(world)...);
    }((args *)nullptr, *this, obj);
    return *this;
};
template <auto System>
inline World &World::run_system() {
    using trait = func_traits<System>;
    using args = trait::args;
    []<typename... Args>(std::tuple<Args...> *,
                         World &world) -> decltype(auto) {
        System(World::fatch_args<Args>(world)...);
    }((args *)nullptr, *this);
    return *this;
}

template <typename Resource, typename... Args>
World &World::add_resource(Args &&...args) {
    resource_manager_.add<Resource>(std::forward<Args>(args)...);
    std::println("add resource: {} , id:{}", typeid(Resource).name(),
                 resource_manager_.id<Resource>());
    return *this;
};
template <typename Resource>
World &World::add_resource(Resource *resource) {
    resource_manager_.add<Resource>(resource);
    std::println("add resource: {} , id:{}", typeid(Resource).name(),
                 resource_manager_.id<Resource>());
    return *this;
};

template <typename Component>
World &World::add_component() {
    XC_ASSERT(
        !component2pool_map_.has_value(ComponentIdGenerator<Component>::get()));
    std::println("add component: {} , id:{}", typeid(Component).name(),
                 pools_.size());
    auto pool_index = component_infos_.size();
    pools_.emplace_back();
    component_infos_.emplace_back(pool_index, [=](void *ptr) {
        delete (static_cast<purge_t<Component> *>(ptr));
    });
    component2pool_map_.insert(ComponentIdGenerator<Component>::get());
    return *this;
}

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
    } else if constexpr (std::is_same_v<Np, World &>) {
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
        return &world.submit();
    } else if constexpr (std::is_same_v<Np, CommandSubmit &>) {
        return std::ref(world.submit());
    } else if constexpr (std::is_same_v<Np, CommandSubmit>) {
        static_assert(false, "use CommandSubmit by ref or ptr");
    } else if constexpr (std::is_same_v<Np, ResourceManager *>) {
        return &world.resource_manager();
    } else if constexpr (std::is_same_v<Np, ResourceManager &>) {
        return std::ref(world.resource_manager());
    } else if constexpr (std::is_same_v<Np, ResourceManager>) {
        static_assert(false, "use ResourceManager by ref or ptr");
    } else if constexpr (std::is_same_v<Np, Pt &>) {
        return std::ref(world.resource<Pt>());
    } else if constexpr (std::is_same_v<Np, const Pt &>) {
        return std::cref(world.resource<Pt>());
    } else if constexpr (std::is_same_v<Np, const Pt *> ||
                         std::is_same_v<Np, const Pt *>) {
        return (const Pt *)&world.resource<Pt>();
    } else if constexpr (std::is_same_v<Np, Pt *> ||
                         std::is_same_v<Np, const Pt *>) {
        return (Pt *)&world.resource<Pt>();
    } else {
        static_assert(false, "not support this type");
    }
}

template <typename... Component, typename Fn, typename... Args>
    requires std::is_invocable_v<Fn, Component &..., Args...>
inline void ComponentAccessor::each(Fn &&fn, Args &&...args) {
    std::array<size_t, sizeof...(Component)> component_info_index_{
        world_.component2pool_map_.get_index(
            ComponentIdGenerator<Component>::get())...};
    auto it = std::min_element(
        component_info_index_.begin(), component_info_index_.end(),
        [&](size_t a, size_t b) {
            XC_ASSERT(a < world_.component_infos_.size());
            XC_ASSERT(b < world_.component_infos_.size());
            return world_.component_infos_[a].entities_.size() <
                   world_.component_infos_[b].entities_.size();
        });
    XC_ASSERT(it != component_info_index_.end());
    XC_ASSERT(*it < world_.component_infos_.size());
    auto &component_info = world_.component_infos_[*it];
    [&]<size_t... I>(std::index_sequence<I...>) {
        for (auto entity : component_info.entities_) {
            if ((world_.component_infos_[component_info_index_[I]].has_entity(
                     Entity(entity, 0)) &&
                 ...))
                fn(*(Component *)world_
                        .pools_
                            [world_.component_infos_[component_info_index_[I]]
                                 .pool_index_]
                            [world_.component_infos_[component_info_index_[I]]
                                 .cell_index(Entity(entity, 0))]
                        .get()...,
                   std::forward<Args>(args)...);
        }
    }(std::make_index_sequence<sizeof...(Component)>());
}
}  // namespace ecs
