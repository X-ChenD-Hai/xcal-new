#pragma once
#include <type_traits>

#include "xc/ecs2/command.hpp"
#include "xc/ecs2/component.hpp"

namespace xc::ecs {
template <typename T>
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
    system_derived_t,                        //
    system_query_t,                          //
    system_create_t,                         //
    system_destroy_t,                        //
    system_attach_t,                         //
    system_detach_t                          //
    >;

template <typename... T>
using base_sys_t = traits::repack_t<
    traits::batch_transform_t<traits::type_record<T...>, system_type_list_t>,
    System>;

template <typename Q, typename Derive>
    requires(traits::is_specialized_v<ComponentQuery, Q> &&
             !std::is_invocable_v<Derive, Q&>)
class System<Q, Derive> : public BaseSystem {
   public:
    using query_t = Q;
    using BaseSys = System<Q>;
    System() = default;
    void set_query_pool(ComponentQueryCachePool* pool) { pool_ = pool; }
    ComponentQueryCachePool* query_pool() { return pool_; }

   protected:
    virtual void execute(ComponentRegistry&) override {}

   public:
    ~System() override = default;
    ComponentQueryCachePool* pool_{nullptr};
};
template <typename Q, typename Fn>
    requires(traits::is_specialized_v<ComponentQuery, Q> &&
             std::is_invocable_v<Fn, Q&>)
class System<Q, Fn> : public BaseSystem {
   public:
    using query_t = Q;
    using BaseSys = System<Q>;
    System(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}
    void set_query_pool(ComponentQueryCachePool* pool) { pool_ = pool; }
    ComponentQueryCachePool* query_pool() { return pool_; }

   protected:
    virtual void execute(ComponentRegistry& reg) override {
        auto q = pool_->query<Q>();
        fn_(q);
    }

   public:
    ~System() override = default;

   private:
    Fn fn_;
    ComponentQueryCachePool* pool_{nullptr};
};

}  // namespace xc::ecs