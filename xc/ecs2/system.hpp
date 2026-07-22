#pragma once
#include <type_traits>

#include "xc/ecs2/component.hpp"

namespace xc::ecs {

class BaseSystem {
   public:
    virtual void execute(ComponentRegistry&) = 0;
    virtual ~BaseSystem() = 0;
};
inline BaseSystem::~BaseSystem() = default;

template <typename Q, typename Derive = void>
class System;
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