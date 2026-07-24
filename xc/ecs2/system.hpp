#pragma once
#include <type_traits>

#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/markers.hpp"
namespace xc::ecs {

template <typename... T>
class System;
class BaseSystem {
   public:
    virtual void execute(ComponentRegistry&) = 0;
    virtual ~BaseSystem() = 0;
};
inline BaseSystem::~BaseSystem() = default;

using system_marker_list_t =
    traits::template_record<ComponentQuery, CreateEntity, DestroyEntity, Attach,
                            Detach, Derived>;

template <typename derive, typename... T>
using derived_system_t = traits::conditional_t<
    traits::count_if_v<System<T...>, traits::is_specialized_from<Derived>>,
    traits::transform_if_t<System<T...>, traits::same_as<Derived<>>,
                           traits::transfer_to<Derived<derive>>>,
    System<T..., Derived<derive>>>;
template <typename... T>
using base_sys_t =
    traits::repack_t<collect_all_markers_t<system_marker_list_t, T...>, System>;

template <typename D>
class System<Derived<D>> : public BaseSystem {
   public:
    using derive_t = D;

   private:
    virtual void execute(ComponentRegistry&) = 0;
};
template <typename... Q, typename... T>
class System<ComponentQuery<Q...>, T...>
    : public derived_system_t<System<ComponentQuery<Q...>, T...>, T...> {
   public:
    using System<T...>::System;
    using query_t = ComponentQuery<Q...>;
    static constexpr bool has_query = sizeof...(Q) > 0;
    void set_query_pool(ComponentQueryCachePool* pool) { pool_ = pool; }
    ComponentQueryCachePool* query_pool() { return pool_; }

    query_t query() { return pool_->query<query_t>(); }

   private:
    ComponentQueryCachePool* pool_{nullptr};
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

template <typename Fn, typename... T>
    requires(std::is_invocable_v<Fn, collect_marker_t<ComponentQuery, T...>&>)
class System<Derived<void, Fn>, T...>
    : public System<T..., Derived<System<Derived<void, Fn>, T...>>> {
   public:
    using base_ = System<T..., Derived<System<Derived<void, Fn>, T...>>>;
    System(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   protected:
    virtual void execute(ComponentRegistry& reg) override {
        auto q = this->query_pool()->template query<typename base_::query_t>();
        fn_(q);
    }

   public:
    ~System() override = default;

   private:
    Fn fn_;
};

}  // namespace xc::ecs