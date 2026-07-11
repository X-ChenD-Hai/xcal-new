#pragma once
#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "types.hpp"

namespace xc::ecs {

template <typename T, typename = void>
constexpr bool is_waitable = false;
template <typename T>
constexpr bool is_waitable<
    T, std::void_t<decltype(void(std::declval<T>().await_ready()))>> = true;

class Worker;

struct PromiseState {
    static constexpr uint32_t DONE = std::numeric_limits<uint32_t>::max();

    alignas(64) std::atomic_uint32_t remain_task_count{};
    std::exception_ptr exception{nullptr};
    const Worker* bind_worker{nullptr};
    BasePromise* promise{nullptr};
    std::weak_ptr<PromiseState> parent{};
    void (*on_child_final_suspend)(std::shared_ptr<PromiseState>){nullptr};
    void* data{nullptr};
    bool done() {
        return remain_task_count.load(std::memory_order_acquire) == DONE;
    }
};

class BasePromise {
    friend class SystemScheduler;
    friend struct ResumeUntilOnceTask;
    friend class PromisLockGuard;
    static constexpr uint32_t DONE = PromiseState::DONE;
    using remain_count_t = std::atomic<uint32_t>;
    template <typename U>
    friend void invoke_task(U&& task);

   public:
    BasePromise() {
        state_ = std::make_shared<PromiseState>();
        state_->promise = this;
    };
    BasePromise(const BasePromise&) = delete;
    BasePromise(BasePromise&&) = delete;
    BasePromise& operator=(const BasePromise&) = delete;
    BasePromise& operator=(BasePromise&&) = delete;

    virtual ~BasePromise() {
        _SCHEDULER_DEBUG("destory Promise {}", (void*)this);
    }

   public:
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept {
        auto parent = state_->parent.lock();
        assert("remain_task_count is not 0" &&
               state_->remain_task_count.load(std::memory_order_acquire) == 0);
        state_->remain_task_count.store(DONE, std::memory_order_release);
        state_->promise = nullptr;
        if (parent) {
            if (parent->on_child_final_suspend) {
                parent->on_child_final_suspend(state_);
            } else if (parent->promise) {
                parent->promise->async_end_wait();
            }
        }
        return {};
    }
    void unhandled_exception() { state_->exception = std::current_exception(); }
    void set_parent(BasePromise* parent) {
        assert(this != parent && "set_parent self");
        if (parent) parent->begin_wait();
        if (auto parent = state_->parent.lock();
            parent && parent->promise != nullptr)
            parent->promise->async_end_wait();
        state_->parent = parent ? parent->state_ : nullptr;
    }
    void add_child(BasePromise& child) {
        child.set_scheduler(scheduler_);
        child.set_parent(this);
        child.async_resume();
    }
    void async_resume();
    void bind(const Worker* worker) { state_->bind_worker = worker; }
    const Worker* bind_worker() const { return state_->bind_worker; }
    inline std::exception_ptr exception() const noexcept {
        return state_->exception;
    }
    inline void set_scheduler(SystemScheduler* scheduler) {
        scheduler_ = scheduler;
    }
    void begin_wait() {
        state_->remain_task_count.fetch_add(1, std::memory_order_acq_rel);
        _SCHEDULER_DEBUG("{} begin wait remain {}", (void*)this,
                         remain_task_count_.load());
    }
    void end_wait() {
        _SCHEDULER_DEBUG("{} end wait remain {}", (void*)this,
                         remain_task_count_.load());
        if (state_->remain_task_count.fetch_sub(1, std::memory_order_acq_rel) ==
            1) {
            _SCHEDULER_DEBUG("{} end wait toggle resume", (void*)this);
            if (!done()) resume();
        }
    }
    void async_end_wait();
    void submit_timeout_task(task_t&& task, time_point_t until);
    void submit_timeout_task(task_t&& task, time_duration_t delay);
    const SystemScheduler* scheduler() const noexcept { return scheduler_; }
    inline SystemScheduler* const& scheduler() noexcept { return scheduler_; }
    inline void set_exception(std::exception_ptr exception) {
        state_->exception = exception;
    }
    std::coroutine_handle<> handle() {
        return std::coroutine_handle<>::from_address(address_);
    }
    void resume() {
        assert(!done() && "handle is done");
        handle().resume();
    }
    bool done() const {
        return state_->remain_task_count.load(std::memory_order_acquire) ==
               DONE;
    }
    std::shared_ptr<PromiseState> state() { return state_; }
    std::shared_ptr<const PromiseState> state() const { return state_; }

   protected:
    std::shared_ptr<PromiseState> state_{std::make_shared<PromiseState>()};
    void* address_{nullptr};
    SystemScheduler* scheduler_{nullptr};
};
template <typename Derived>
class Promise : public BasePromise {
   public:
    using BasePromise::BasePromise;
    using handle_t = std::coroutine_handle<Derived>;

    template <typename T>
    auto await_transform(T&& t) {
        if constexpr (is_waitable<T>) {
            return std::forward<T>(t);
        } else {
            using Tp = std::decay_t<T>;
            if constexpr (is_waitable<decltype(std::forward<T>(t).get_awaitable(
                              scheduler_, handle()))>) {
                return std::forward<T>(t).get_awaitable(scheduler_, handle());
            } else if constexpr (is_waitable<decltype(T::get_awaitable(
                                     std::forward<T>(t), scheduler_,
                                     handle()))>) {
                return T::get_awaitable(std::forward<T>(t), scheduler_,
                                        handle());
            }
        }
    }

    handle_t handle() const { return handle_t::from_address(address_); }

    void resubmit();

    void submit_task(task_t&& task);

   protected:
    handle_t init_handle() {
        auto handle = handle_t::from_promise(*static_cast<Derived*>(this));
        address_ = handle.address();
        return handle;
    }
};
class PromisLockGuard {
   public:
    PromisLockGuard() = delete;

    PromisLockGuard(BasePromise& promise) : promise_(&promise) {
        promise_->begin_wait();
    }
    PromisLockGuard(BasePromise* promise) : promise_(promise) {
        promise_->begin_wait();
    }
    ~PromisLockGuard() { release(); }
    void release() {
        if (promise_) promise_->end_wait();
        promise_ = nullptr;
    }

   public:
    PromisLockGuard(const PromisLockGuard&) = delete;
    PromisLockGuard(PromisLockGuard&&) = delete;
    PromisLockGuard& operator=(const PromisLockGuard&) = delete;
    PromisLockGuard& operator=(PromisLockGuard&&) = delete;

   private:
    BasePromise* promise_{nullptr};
};
}  // namespace xc::ecs