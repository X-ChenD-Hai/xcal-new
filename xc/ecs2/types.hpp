#pragma once

#include <coroutine>
#include <functional>
#include <variant>
namespace xc::ecs {
class SystemScheduler;
class System;
class SystemPromise;
class Worker;
class Task {
    friend class SystemPromise;
    using task_t = std::function<void(void)>;

   public:
    Task() = default;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;

   public:
    Task(std::function<void(void)> task) : task_(task), bind_(nullptr) {}
    Task(std::function<void(void)> task, Worker* bind)
        : task_(task), bind_(bind) {}
    void bind(Worker* bind) noexcept { bind_ = bind; }
    const Worker* bind_worker() const noexcept { return bind_; }
    void operator()() const { task_(); }
    explicit operator bool() const { return (bool)task_; }

   private:
    task_t task_{};
    Worker* bind_{nullptr};
};
using system_handle_t = std::coroutine_handle<SystemPromise>;
using task_t = std::variant<Task, system_handle_t>;

template <typename U>
const Worker* bind_worker(const U& task) {
    return std::visit(
        [](auto&& t) {
            using T = std::decay_t<decltype(t)>;
            if constexpr (std::is_same_v<T, Task>) {
                return t.bind_worker();

            } else {
                return t.promise().bind_worker();
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
                assert(!task.done() && "handle is invalid");
                _SCHEDULER_DEBUG("start resume {} from submited",
                                 task.address());
                try {
                    task.resume();
                    _SCHEDULER_DEBUG("resume {} success with id {}",
                                     task.address(), task.promise().id);
                } catch (...) {
                    task.promise().exception_ = std::current_exception();
                    _SCHEDULER_DEBUG("catch exception in {}", task.address());
                }
            }
        },
        task);
}

}  // namespace xc::ecs