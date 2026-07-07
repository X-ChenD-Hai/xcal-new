#pragma once
#include <atomic>
#include <coroutine>
#include <exception>
#include <utility>

#include "types.hpp"

namespace xc::ecs {
class Worker;
class BasePromise {
    friend class SystemScheduler;
    friend struct ResumeUntilOnceTask;
    friend struct ResumeUntilDoneTask;

    template <typename U>
    friend void invoke_task(U&& task);

   public:
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void set_parent(BasePromise* parent) { parent_ = parent; }
    void unhandled_exception() { exception_ = std::current_exception(); }
    void bind(const Worker* worker) { bind_worker_ = worker; }
    const Worker* bind_worker() const { return bind_worker_; }
    inline std::exception_ptr exception() const noexcept { return exception_; }
    inline void set_scheduler(SystemScheduler* scheduler) {
        scheduler_ = scheduler;
    }
    virtual std::coroutine_handle<> handle() = 0;
    void begin_wait() {
        remain_task_count_.fetch_add(1);
        _SCHEDULER_DEBUG("{} begin wait remain {}", (void*)this,
                     remain_task_count_.load());
    }
    void end_wait() {
        _SCHEDULER_DEBUG("{} end wait remain {}", (void*)this,
                     remain_task_count_.load());
        if (remain_task_count_.fetch_sub(1) == 1) {
            _SCHEDULER_DEBUG("{} end wait toggle resume", (void*)this);
            if (!handle().done()) handle().resume();
            if (parent_) parent_->end_wait();
        }
    }

   protected:
    SystemScheduler* scheduler_{nullptr};
    std::exception_ptr exception_{nullptr};
    const Worker* bind_worker_{nullptr};
    std::atomic_uint32_t remain_task_count_{};
    BasePromise* parent_{nullptr};
};
template <typename Derive>
class Promise : public BasePromise {
   public:
    using handle_t = std::coroutine_handle<Derive>;
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

    std::coroutine_handle<> handle() override { return handle_; }

    void resubmit();

    void submit_task(task_t&& task);

   protected:
    handle_t handle_{nullptr};
};

}  // namespace xc::ecs