#pragma once
#include <atomic>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>

#include "./config.hpp"
#include "./promise.hpp"
#include "./types.hpp"

namespace xc::ecs {
class SystemPromise;
class SystemScheduler;
using system_handle_t = std::coroutine_handle<SystemPromise>;
class System {
   public:
    using promise_type = SystemPromise;
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = default;
    System& operator=(System&&) = default;
    System() noexcept {
        _SCHEDULER_DEBUG("default construct system {}", (void*)this);
    };
    System(std::shared_ptr<PromiseState> state) noexcept : state_(state) {
        _SCHEDULER_DEBUG("construct system {} @ {}", (void*)this,
                         (void*)state_->promise);
    }
    ~System();
    std::shared_ptr<PromiseState> state_{};
};
template <typename T>
concept IsPromise = std::derived_from<T, Promise<T>>;

class SystemPromise : public Promise<SystemPromise> {
   public:
    friend class SystemScheduler;
    friend void invoke_task(task_t&& task);
    System get_return_object() {
        address_ = handle_t::from_promise(*this).address();
        return System{state_};
    };
    void return_void() {}
};
inline System::~System() {
    _SCHEDULER_DEBUG("destroy system {} @ {}", (void*)this,
                     (void*)(state_ ? state_->promise : nullptr));
}
}  // namespace xc::ecs