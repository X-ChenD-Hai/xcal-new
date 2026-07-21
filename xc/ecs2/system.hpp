#pragma once

#include "xc/ecs2/component.hpp"

namespace xc::ecs {

class BaseSystem {
   public:
    virtual void execute(ComponentRegistry&) = 0;
    virtual ~BaseSystem() = 0;
};
inline BaseSystem::~BaseSystem() = default;

template <typename Q>
class System;
template <typename Q>
    requires(details::is_specialized_v<ComponentQuery, Q>)
class System<Q> : public BaseSystem {
   public:
    using query_t = Q;
    using BaseSys = System<Q>;
    System() = default;

   protected:
    virtual void execute(ComponentRegistry&) override {}

   public:
    ~System() override = default;
};
}  // namespace xc::ecs