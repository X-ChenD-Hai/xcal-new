#pragma once
#include <concepts>
#include <functional>
#include <type_traits>

#include "promise.hpp"
#include "types.hpp"

namespace xc::ecs {
struct ResumeUntilOnceTask {
    std::coroutine_handle<> handle{nullptr};
    BasePromise* promise{nullptr};
    template <typename H>
    ResumeUntilOnceTask(std::coroutine_handle<H> h)
        : handle(h), promise(&h.promise()) {}
    ~ResumeUntilOnceTask() {}
    void operator()() const {
        try {
            handle.resume();
            _SCHEDULER_DEBUG("resume {} success from until once handle",
                             (void*)promise);
        } catch (...) {
            promise->exception_ = std::current_exception();
            _SCHEDULER_DEBUG("catch exception in {}", (void*)promise);
        }
    }
};
struct ResumeUntilDoneTask : public ResumeUntilOnceTask {
    using ResumeUntilOnceTask::ResumeUntilOnceTask;
    void operator()() const {
        try {
            promise->begin_wait();
            handle.resume();
            _SCHEDULER_DEBUG("resume {} success from until done handle",
                             (void*)promise);
            promise->end_wait();
        } catch (...) {
            promise->exception_ = std::current_exception();
            _SCHEDULER_DEBUG("catch exception in {}", (void*)promise);
            promise->end_wait();
        }
    }
};

class FuncTask {
    template <typename T>
    friend class Promise;
    using task_t = std::function<void(void)>;

   public:
    FuncTask() = default;
    FuncTask(const FuncTask&) = delete;
    FuncTask& operator=(const FuncTask&) = delete;
    FuncTask(FuncTask&&) = default;
    FuncTask& operator=(FuncTask&&) = default;

   public:
    FuncTask(std::function<void(void)> task) : task_(task), bind_(nullptr) {}
    FuncTask(std::function<void(void)> task, const Worker* bind)
        : task_(task), bind_(bind) {}
    void bind(Worker* bind) noexcept { bind_ = bind; }
    const Worker* bind_worker() const noexcept { return bind_; }
    void operator()() { task_(); }
    explicit operator bool() const { return (bool)task_; }

   private:
    task_t task_{};
    const Worker* bind_{nullptr};
};
template <typename T, typename = void>
constexpr bool is_visitable = false;
template <typename T>
constexpr bool
    is_visitable<T, decltype(std::visit([](auto&&) {}, std::declval<T>()))> =
        true;

template <typename U>
const Worker* bind_worker(const U& task) {
    using T = std::decay_t<U>;
    if constexpr (is_visitable<U>) {
        return std::visit([](auto&& t) { return bind_worker(t); }, task);
    } else if constexpr (std::is_same_v<T, FuncTask>) {
        return task.bind_worker();
    } else if constexpr (std::derived_from<T, ResumeUntilOnceTask>) {
        return task.promise->bind_worker();
    } else if constexpr (std::derived_from<T, BasePromise>) {
        return task.bind_worker();
    } else {
        static_assert(false, "task type not supported");
    }
}

template <typename U>
void invoke_task(U&& task) {
    if constexpr (is_visitable<U>) {
        std::visit([](auto&& task) { invoke_task(task); }, task);
    } else {
        std::forward<U>(task)();
    }
}

}  // namespace xc::ecs
