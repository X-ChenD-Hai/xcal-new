#pragma once
#include <tuple>
#include <type_traits>

#include "ecs2/resource.hpp"
#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/markers.hpp"
namespace xc::ecs {

template <typename... T>
class System;
class BaseSystem {
   public:
    struct Tags {
        struct SelfRef;
    };
    struct ImplPolicy {
        template <typename... Args>
        struct Function;
    };

   public:
    virtual void vtl_execute() = 0;
    virtual void vtl_set_query_pool(ComponentQueryCachePool* pool) {}
    virtual void vtl_set_resource_registry(ResourceRegistry* reg) {}
    virtual void vtl_set_component_registry(ComponentRegistry* reg) {}

    virtual ~BaseSystem() = 0;
};
inline BaseSystem::~BaseSystem() = default;

using system_marker_list_t =
    traits::template_record<ComponentQuery, CreateEntity, DestroyEntity, Attach,
                            Detach, Resource, Derived>;

template <typename derive, typename... T>
using derived_system_t = traits::conditional_t<
    traits::count_if_v<System<T...>, traits::is_specialized_from<Derived>>,
    traits::transform_if_t<System<T...>, traits::same_as<Derived<>>,
                           traits::transfer_to<Derived<derive>>>,
    System<T..., Derived<derive>>>;
template <typename... T>
using base_sys_t =
    traits::repack_t<collect_all_markers_t<system_marker_list_t, T...>, System>;

template <typename D, typename = void>
struct has_execute : std::false_type {};
template <typename D>
struct has_execute<D, std::void_t<decltype(std::declval<D>().execute())>>
    : std::true_type {};
template <typename D>
constexpr bool has_execute_v = has_execute<D>::value;

template <typename D, typename = void>
class DerivedBaseSystem : public BaseSystem {
    void vtl_execute() override {
        if constexpr (has_execute_v<D>) {
            static_cast<D*>(this)->execute();
        } else {
            static_assert(false,
                          R"(
                please override execute() 
                and transfer a derived system
                e.g:
                class Sys public :System<ComponentQuery<>, Derived<Sys>> {
                                                           ^^^^^^^^^^^^^
                                                    transfer a derived system
                    public: // must be public
                        void execute() {
                            // do somethings
                        }
                    }
                )");
        }
    }
};

template <typename D>
class System<Derived<D>> : public DerivedBaseSystem<D> {
   public:
    using derive_t = D;
};
template <typename... T>
class System<ComponentQuery<>, T...>
    : public derived_system_t<System<ComponentQuery<>, T...>, T...> {
   public:
    using query_t = ComponentQuery<>;
};
template <typename... Q, typename... T>
class System<ComponentQuery<Q...>, T...>
    : public derived_system_t<System<ComponentQuery<Q...>, T...>, T...> {
   public:
    using query_t = ComponentQuery<Q...>;
    static constexpr bool has_query = sizeof...(Q) > 0;
    void vtl_set_query_pool(ComponentQueryCachePool* pool) override {
        pool_ = pool;
    }
    ComponentQueryCachePool* query_pool() { return pool_; }

    query_t query() { return pool_->query<query_t>(); }

   private:
    ComponentQueryCachePool* pool_{nullptr};
};
template <typename... T>
class System<Resource<>, T...>
    : public derived_system_t<System<Resource<>, T...>, T...> {
   public:
    using resource_accessor_t = ResourceAccessor<>;
};
template <typename... R, typename... T>
class System<Resource<R...>, T...>
    : public derived_system_t<System<Resource<R...>, T...>, T...> {
   public:
    using derived_system_t<System<Resource<R...>, T...>,
                           T...>::derived_system_t;
    using resource_accessor_t = ResourceAccessor<R...>;
    void vtl_set_resource_registry(ResourceRegistry* reg) override {
        reg_ = reg;
    }
    resource_accessor_t resource() { return {*reg_}; }

   private:
    ResourceRegistry* reg_{nullptr};
};
template <typename... C, typename... T>
class System<CreateEntity<C...>, T...>
    : public derived_system_t<System<CreateEntity<C...>, T...>, T...> {};
template <typename... D, typename... T>
class System<DestroyEntity<D...>, T...>
    : public derived_system_t<System<DestroyEntity<D...>, T...>, T...> {};
template <typename... A, typename... T>
class System<Attach<A...>, T...>
    : public derived_system_t<System<Attach<A...>, T...>, T...> {};
template <typename... D, typename... T>
class System<Detach<D...>, T...>
    : public derived_system_t<System<Detach<D...>, T...>, T...> {};
template <typename... T>
class System : public base_sys_t<T...> {};

template <typename Sys, typename ArgPack = std::tuple<>, typename = void>
struct SystemCreator {
    using system_t = Sys;
    template <typename... Args>
    static system_t* create(Args&&... args) {
        return new system_t(std::forward<Args>(args)...);
    }
};
template <typename... O>
struct is_abstract_system
    : std::negation<traits::contains_if<
          System<O...>,
          traits::conjunction<traits::is_specialized_from<Derived>,
                              traits::negation<traits::same_as<Derived<>>>>>> {
};
template <typename... Q>
constexpr bool is_abstract_system_v = is_abstract_system<Q...>::value;

template <typename... Q, typename Fn>
struct SystemCreator<
    System<Q...>, std::tuple<Fn>,
    std::enable_if_t<is_abstract_system_v<Q...> && traits::is_function_v<Fn>>> {
    using system_t = System<traits::repack_t<traits::function_args_t<Fn>,
                                             BaseSystem::ImplPolicy::Function>,
                            Fn, Q...>;
    static system_t* create(Fn&& args) {
        return new system_t(std::forward<Fn>(args));
    }
};
template <typename... Q, typename Fn>
struct SystemCreator<
    System<Q...>, std::tuple<Fn>,
    std::enable_if_t<
        is_abstract_system_v<Q...> && !traits::is_function_v<Fn> &&
        std::is_invocable_v<
            Fn,
            base_sys_t<Q..., Derived<System<BaseSystem::ImplPolicy::Function<
                                                BaseSystem::Tags::SelfRef>,
                                            Fn, Q...>>>&>>> {
    using system_t =
        System<BaseSystem::ImplPolicy::Function<BaseSystem::Tags::SelfRef>, Fn,
               Q...>;
    static system_t* create(Fn&& args) {
        return new system_t(std::forward<Fn>(args));
    }
};
template <typename... Q, typename... Args, typename Fn>
class System<BaseSystem::ImplPolicy::Function<Args...>, Fn, Q...>
    : public base_sys_t<
          Q..., Derived<System<BaseSystem::ImplPolicy::Function<Args...>, Fn,
                               Q...>>> {
   public:
    using base_t = base_sys_t<
        Q...,
        Derived<System<BaseSystem::ImplPolicy::Function<Args...>, Fn, Q...>>>;
    System(Fn&& fn)
        : fn_(std::forward<Fn>(fn))

    {}

   protected:
    void vtl_execute() override {
        if constexpr (std::is_invocable_v<Fn, base_t&>) {
            fn_(*(base_t*)this);
        } else {
            fn_(this);
        }
    }

   public:
    ~System() override = default;

   private:
    Fn fn_;
};

}  // namespace xc::ecs