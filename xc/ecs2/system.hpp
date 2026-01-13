#pragma once
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "config.hpp"
#include "types.hpp"

namespace xc::ecs {
struct SystemPromise;
struct SystemScheduler;
struct System {
    using promise_type = SystemPromise;
    System(System&& o) : handle(nullptr) {
        _SCHEDULER_DEBUG("move construct system {} @ {}", (void*)this,
                         (void*)o.handle.address());
        std::swap(handle, o.handle);
    }
    System& operator=(System&& o) {
        _SCHEDULER_DEBUG("move assign system {} @ {}", (void*)this,
                         (void*)o.handle.address());
        std::swap(handle, o.handle);
        return *this;
    }
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System() noexcept : handle(nullptr) {
        _SCHEDULER_DEBUG("default construct system {}", (void*)this);
    };
    System(system_handle_t handle) noexcept : handle(handle) {
        _SCHEDULER_DEBUG("construct system {} @ {}", (void*)this,
                         (void*)handle.address());
    }
    ~System();
    system_handle_t handle{nullptr};
};
struct SystemPromise {
    System get_return_object() {
        return System{handle_ = system_handle_t::from_promise(*this)};
    };
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always await_suspend(
        std::coroutine_handle<SystemPromise> caller) noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() {
        exception_ = std::current_exception();
        // waiting.test_and_set();
    }

    template <typename T>
    auto yield_value(T&& t) {
        if constexpr (std::is_member_function_pointer_v<decltype(&T::yield)>) {
            return t.yield(scheduler_, handle_);
        } else {
            return T::yield(std::forward<T>(t), scheduler_, handle_);
        }
    };
    template <typename T>
    auto await_transform(T&& t) {
        return std::decay_t<T>::wait_type::await_transform(std::forward<T>(t), scheduler_, handle_);
    }
    template <typename T>
    auto await_transform(T& t) {
        return std::decay_t<T>::wait_type::await_transform(std::forward<T>(t), scheduler_, handle_);
    }
    template <typename T>
    auto await_transform(const T& t) {
        return std::decay_t<T>::wait_type::await_transform(std::forward<T>(t), scheduler_, handle_);
    }

    void return_void() {}
    SystemScheduler* scheduler_{nullptr};
    std::exception_ptr exception_{nullptr};
    uint32_t id{0};
    system_handle_t handle_;
};
inline System::~System() {
    _SCHEDULER_DEBUG("destroy system {} @ {} {}", (void*)this,
                     (void*)handle.address(),
                     handle ? handle.promise().id : NAN);
    assert((!handle || handle.done() || handle.promise().exception_) &&
           "system is not done");
    if (handle) handle.destroy();
}
}  // namespace xc::ecs