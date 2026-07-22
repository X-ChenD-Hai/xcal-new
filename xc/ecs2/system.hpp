#pragma once
#include <type_traits>

#include "ecs2/comman/traits.hpp"
#include "xc/ecs2/command.hpp"
#include "xc/ecs2/component.hpp"

namespace xc::ecs {
template <typename... T>
class Derived;
template <typename... T>
class System;
class BaseSystem {
   public:
    virtual void execute(ComponentRegistry&) = 0;
    virtual ~BaseSystem() = 0;
};
inline BaseSystem::~BaseSystem() = default;

template <typename... T>
using system_derived_t =
    traits::collect_marker_t<false, Derived, traits::template_record<>, T...>;
template <typename... T>
using system_query_t =
    traits::collect_marker_t<false, ComponentQuery, traits::template_record<>,
                             T...>;
template <typename... T>
using system_create_t =
    traits::collect_marker_t<false, CreateEntity, traits::template_record<>,
                             T...>;
template <typename... T>
using system_destroy_t =
    traits::collect_marker_t<false, DestroyEntity, traits::template_record<>,
                             T...>;
template <typename... T>
using system_attach_t =
    traits::collect_marker_t<false, Attach, traits::template_record<>, T...>;
template <typename... T>
using system_detach_t =
    traits::collect_marker_t<false, Detach, traits::template_record<>, T...>;
using system_type_list_t = template_record<  //
    system_query_t,                          //
    system_create_t,                         //
    system_destroy_t,                        //
    system_attach_t,                         //
    system_detach_t,                         //
    system_derived_t                         //
    >;

template <typename... T>
using base_sys_t = traits::repack_t<
    traits::batch_transform_t<traits::type_record<std::conditional_t<
                                  traits::is_specialized_v<Derived, T>,
                                  std::conditional_t<traits::size_of_v<T> != 1,
                                                     Derived<System<T...>>, T>,
                                  T>...>,
                              system_type_list_t>,
    System>;

template <typename D>
class System<Derived<D>> : public BaseSystem {
   public:
    using derive_t = D;

   private:
    virtual void execute(ComponentRegistry&) = 0;
};
template <typename... Q, typename... T>
class System<ComponentQuery<Q...>, T...> : public System<T...> {
   public:
    using System<T...>::System;
    using query_t = ComponentQuery<Q...>;
    static constexpr bool has_query = sizeof...(Q) > 0;
    void set_query_pool(ComponentQueryCachePool* pool) { pool_ = pool; }
    ComponentQueryCachePool* query_pool() { return pool_; }

   private:
    ComponentQueryCachePool* pool_{nullptr};
};
template <typename... C, typename... T>
class System<CreateEntity<C...>, T...> : public System<T...> {};
template <typename... D, typename... T>
class System<DestroyEntity<D...>, T...> : public System<T...> {};
template <typename... A, typename... T>
class System<Attach<A...>, T...> : public System<T...> {};
template <typename... D, typename... T>
class System<Detach<D...>, T...> : public System<T...> {};
template <typename... T>
class System : public base_sys_t<T...> {};

// template <typename Q, typename Derive>
//     requires(traits::is_specialized_v<ComponentQuery, Q> &&
//              !std::is_invocable_v<Derive, Q&>)
// class System<Q, Derive> : public BaseSystem {
//    public:
//     using query_t = Q;
//     using BaseSys = System<Q>;
//     System() = default;
//     void set_query_pool(ComponentQueryCachePool* pool) { pool_ = pool; }
//     ComponentQueryCachePool* query_pool() { return pool_; }

//    protected:
//     virtual void execute(ComponentRegistry&) override {}

//    public:
//     ~System() override = default;
//     ComponentQueryCachePool* pool_{nullptr};
// };
template <typename Fn, typename... T>
    requires(std::is_invocable_v<Fn, system_query_t<T...>&>)
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