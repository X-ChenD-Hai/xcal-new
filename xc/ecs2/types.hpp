#pragma once

#include <coroutine>
#include <functional>
#include <variant>
namespace xc::ecs {
class SystemScheduler;
class System;
class SystemPromise;
class Worker;
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
    FuncTask(std::function<void(void)> task, Worker* bind)
        : task_(task), bind_(bind) {}
    void bind(Worker* bind) noexcept { bind_ = bind; }
    const Worker* bind_worker() const noexcept { return bind_; }
    void operator()() const { task_(); }
    explicit operator bool() const { return (bool)task_; }

   private:
    task_t task_{};
    Worker* bind_{nullptr};
};
class BasePromise;
struct HandleTask {
    std::coroutine_handle<> handle{nullptr};
    BasePromise* promise{nullptr};
    template <typename H>
    HandleTask(std::coroutine_handle<H> h) : handle(h), promise(&h.promise()) {}
};
using system_handle_t = std::coroutine_handle<SystemPromise>;
using task_t = std::variant<FuncTask, HandleTask>;

template <typename U>
const Worker* bind_worker(const U& task) {
    return std::visit(
        [](auto&& t) {
            using T = std::decay_t<decltype(t)>;
            if constexpr (std::is_same_v<T, FuncTask>) {
                return t.bind_worker();
            } else {
                return t.promise->bind_worker();
            }
        },
        task);
}
template <typename U>
inline void invoke_task(U&& task) {
    std::visit(
        [](auto&& task) {
            using T = std::decay_t<decltype(task)>;
            if constexpr (std::is_invocable_v<T>) {
                task();
            } else {
                assert(!task.handle.done() && "handle is invalid");
                _SCHEDULER_DEBUG("start resume {} from submited",
                                 task.handle.address());
                try {
                    task.handle.resume();
                    _SCHEDULER_DEBUG("resume {} success",
                                     task.handle.address());
                } catch (...) {
                    task.promise->exception_ = std::current_exception();
                    _SCHEDULER_DEBUG("catch exception in {}", task.handle.address());
                }
            }
        },
        task);
}

}  // namespace xc::ecs