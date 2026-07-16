#pragma once
#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>
#include <variant>

#include "config.hpp"
#include "promise.hpp"
#include "types.hpp"

namespace xc::ecs {
namespace details {
template <typename U>
const Worker* bind_worker(const U& task);
template <typename U>
struct TaskBindWorker;
}  // namespace details
template <typename Task>
class CancelAbleTask;
class CancelToken {
    friend class CancelTask;
    template <typename Task>
    friend class CancelAbleTask;

   public:
    CancelToken(const CancelToken&) = default;
    CancelToken& operator=(const CancelToken&) = default;
    CancelToken(CancelToken&&) = default;
    CancelToken& operator=(CancelToken&&) = default;
    ~CancelToken() = default;

   public:
    CancelToken() : finished_(std::make_shared<std::atomic_flag>()) {
        finished_->clear(std::memory_order_relaxed);
    }

    bool finished_or_caneled() const noexcept {
        return finished_->test(std::memory_order_acquire);
    }

    bool consume() const noexcept {
        return !finished_->test_and_set(std::memory_order_acq_rel);
    }

   private:
    std::shared_ptr<std::atomic_flag> finished_{
        std::make_shared<std::atomic_flag>()};
};
template <typename Task>
class CancelAbleTask {
    template <typename U>
    friend struct ::xc::ecs::details::TaskBindWorker;
    friend class CancelTask;

   public:
    CancelAbleTask() = delete;
    CancelAbleTask(Task&& task) : task_(std::move(task)) {}
    CancelAbleTask(Task&& task, const CancelToken& token)
        : token_(token), task_(std::move(task)) {}
    CancelToken token() const { return token_; }
    void set_parent(BasePromise* parent) { task_.set_parent(parent); }
    void operator()() {
        if (token_.consume())
            task_();
        else
            _SCHEDULER_DEBUG("cancel {} success from func handle", (void*)this);
    }
    inline Task& task() { return task_; }

   private:
    CancelToken token_{};
    Task task_{};
};

class CancelTask {
   public:
    CancelTask() = delete;

    template <typename T>
    CancelTask(const CancelAbleTask<T>& task) : token_(task.token()) {}

    void set_parent(BasePromise* parent) { promise_ = parent; }
    void set_target_parent(BasePromise* parent) { target_parent_ = parent; }
    bool cancel() {
        auto consumed = consume();
        if (consumed) {
            if (target_parent_) target_parent_->async_end_wait();
        }
        if (promise_) promise_->end_wait();
        promise_ = nullptr;
        target_parent_ = nullptr;
        return !consumed;
    }

    void operator()() { cancel(); }

   protected:
    inline bool consume() { return token_.consume(); }

   protected:
    CancelToken token_{};
    BasePromise* promise_{nullptr};
    BasePromise* target_parent_{nullptr};
};
class CancelWithRollbackTask : public CancelTask {
   public:
    template <typename T>
    CancelWithRollbackTask(const CancelAbleTask<T>& task,
                           std::function<void(void)> rollback)
        : CancelTask(task),
          rollback_(std::make_unique<std::function<void(void)>>(rollback)) {}
    bool cancel() {
        auto consumed = consume();
        if (consumed) {
            (*rollback_)();
            if (target_parent_) target_parent_->async_end_wait();
        }
        if (promise_) promise_->end_wait();
        promise_ = nullptr;
        target_parent_ = nullptr;
        return !consumed;
    }
    void operator()() { cancel(); }

   private:
    std::unique_ptr<std::function<void(void)>> rollback_;
};

template <typename T>
constexpr bool is_concelable_task = false;
template <typename T>
constexpr bool is_concelable_task<CancelAbleTask<T>> = true;

struct ResumeUntilOnceTask {
    BasePromise* promise{nullptr};
    ResumeUntilOnceTask(BasePromise* promise) : promise(promise) {}
    ~ResumeUntilOnceTask() {}
    void set_parent(BasePromise* parent) { promise->set_parent(parent); }
    void set_exception(std::exception_ptr e) const {
        promise->set_exception(e);
    }
    void operator()() const {
        assert("task is done" && !promise->done());
        promise->resume();
        _SCHEDULER_DEBUG("resume {} success from until once handle",
                         (void*)promise);
    }
};
struct AsyncEndWaitTask : public ResumeUntilOnceTask {
    using ResumeUntilOnceTask::ResumeUntilOnceTask;
    void operator()() const { promise->end_wait(); }
};
class FuncTask : public ResumeUntilOnceTask {
    template <typename T>
    friend class Promise;
    using task_t = std::function<void(void)>;

   public:
    FuncTask() : ResumeUntilOnceTask(nullptr) {}
    FuncTask(const FuncTask&) = delete;
    FuncTask& operator=(const FuncTask&) = delete;
    FuncTask(FuncTask&&) = default;
    FuncTask& operator=(FuncTask&&) = default;

   public:
    FuncTask(std::function<void(void)> task)
        : ResumeUntilOnceTask(nullptr),
          task_(std::make_unique<task_t>(task)),
          bind_(nullptr) {}
    FuncTask(std::function<void(void)> task, const Worker* bind)
        : ResumeUntilOnceTask(nullptr),
          task_(std::make_unique<task_t>(task)),
          bind_(bind) {}
    void bind(Worker* bind) noexcept { bind_ = bind; }
    const Worker* bind_worker() const noexcept { return bind_; }
    void set_parent(BasePromise* parent) { promise = parent; }
    void operator()() {
        (*task_)();
        promise->end_wait();
    }
    explicit operator bool() const { return (bool)task_ && (bool)(*task_); }

   private:
    std::unique_ptr<task_t> task_{};
    const Worker* bind_{nullptr};
};
template <typename T, typename = void>
constexpr bool is_visitable = false;
template <typename T>
constexpr bool
    is_visitable<T, decltype(std::visit([](auto&&) {}, std::declval<T>()))> =
        true;
template <typename U>
inline const std::string_view task_type(const U& t) {
    if constexpr (is_visitable<U>) {
        return std::visit([](auto&& t) { return typeid(t).name(); }, t);
    } else {
        return typeid(t).name();
    }
}
namespace details {
template <typename U>
const Worker* bind_worker(const U& task);
template <typename U>
struct TaskBindWorker {
    using type = std::decay_t<U>;
    static const Worker* bind_worker(const U& task) {
        return ::xc::ecs::details::bind_worker<type>(task);
    }
};
template <typename U>
struct TaskBindWorker<CancelAbleTask<U>> {
    using type = std::decay_t<U>;
    static const Worker* bind_worker(const CancelAbleTask<U>& task) {
        return ::xc::ecs::details::bind_worker<type>(task.task_);
    }
};
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
        return TaskBindWorker<U>::bind_worker(task);
    }
}

}  // namespace details

using details::bind_worker;

template <typename U>
void invoke_task(U&& task) {
    _SCHEDULER_DEBUG("invoke task {}", task_type(task));
    if constexpr (is_visitable<U>) {
        std::visit([](auto&& task) { invoke_task(task); }, task);
    } else {
        std::forward<U>(task)();
    }
}

}  // namespace xc::ecs
