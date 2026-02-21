#pragma once
#include <atomic>
#include <cassert>
#include <coroutine>
#include <exception>
#include <type_traits>

#include "./config.hpp"
#include "./types.hpp"

namespace xc::ecs {
class SystemPromise;
class SystemScheduler;
class System {
   public:

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
class SystemPromise {
   public:
    friend class SystemScheduler;
    System get_return_object() {
        return System{handle_ = system_handle_t::from_promise(*this)};
    };
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always await_suspend(
        std::coroutine_handle<SystemPromise> caller) noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { exception_ = std::current_exception(); }

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
        return typename std::decay_t<T>::wait_type{std::forward<T>(t),
                                                   scheduler_, handle_};
    }
    template <typename T>
    auto await_transform(T& t) {
        return typename std::decay_t<T>::wait_type{std::forward<T>(t),
                                                   scheduler_, handle_};
    }
    template <typename T>
    auto await_transform(const T& t) {
        return typename std::decay_t<T>::wait_type{std::forward<T>(t),
                                                   scheduler_, handle_};
    }

    void return_void() {}
    void resubmit();
    inline void set_scheduler(SystemScheduler* scheduler) {
        scheduler_ = scheduler;
    }
    void submit_task(const task_t& task);
    void begin_wait() { remain_task_count_.fetch_add(1); }
    void end_wait() {
        if (remain_task_count_.fetch_sub(1) == 1) {
            handle_.resume();
        }
    }
    std::exception_ptr exception() const { return exception_; }

   private:
    SystemScheduler* scheduler_{nullptr};
    std::exception_ptr exception_{nullptr};
    std::atomic_uint32_t remain_task_count_{};
    system_handle_t handle_{nullptr};
};
inline System::~System() {
    _SCHEDULER_DEBUG("destroy system {} @ {} {}", (void*)this,
                     (void*)handle.address(),
                     handle ? handle.promise().id : NAN);
    assert((!handle || handle.done() || handle.promise().exception()) &&
           "system is not done");
    if (handle) handle.destroy();
}
}  // namespace xc::ecs