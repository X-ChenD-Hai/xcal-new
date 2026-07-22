#pragma once

#include <functional>
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

   protected:
    virtual void execute(ComponentRegistry&) override {}

   public:
    ~System() override = default;
};
template <typename Q, typename Fn>
    requires(traits::is_specialized_v<ComponentQuery, Q> &&
             std::is_invocable_v<Fn, Q&>)
class System<Q, Fn> : public BaseSystem {
   public:
    using query_t = Q;
    using BaseSys = System<Q>;
    System(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   protected:
    virtual void execute(ComponentRegistry& reg) override {
        Q q{reg};
        fn_(q);
    }

   public:
    ~System() override = default;

   private:
    Fn fn_;
};

}  // namespace xc::ecs