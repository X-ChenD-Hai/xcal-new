#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>

#include "ecs2/scheduler.hpp"
#include "ecs2/types.hpp"

namespace xc::ecs {

template <typename From>
struct AwaitProtocal {
    using from_type = From;
    using wait_type = From::wait_type;
    AwaitProtocal() = default;
    AwaitProtocal(const AwaitProtocal&) = default;
    AwaitProtocal(AwaitProtocal&&) = default;
    AwaitProtocal& operator=(const AwaitProtocal&) = default;
    AwaitProtocal& operator=(AwaitProtocal&&) = default;
    uint32_t task_count(SystemScheduler* scheduler) { return 1; }
    bool await_ready() { return false; }
    void await_suspend(system_handle_t caller) {
        caller.promise().scheduler_->submit_task([this, caller]() {
            if constexpr (std::is_invocable_v<decltype(&wait_type::task),
                                              wait_type*>) {
                static_cast<wait_type*>(this)->task();
                caller.resume();
            } else if constexpr (std::is_invocable_v<decltype(&wait_type::task),
                                                     wait_type*,
                                                     decltype(caller)>) {
                static_cast<wait_type*>(this)->task(caller);
            } else if constexpr (std::is_invocable_v<
                                     decltype(&wait_type::tasks), wait_type*,
                                     SystemScheduler*>) {
                static_cast<wait_type*>(this)->tasks(
                    caller.promise().scheduler_);
            } else {
                static_assert(false,
                              "wait_type::task 必须是 void(wait_type*) 或 "
                              "void(wait_type*, system_handle_t)");
            }
        });
    }
    decltype(auto) await_resume() {
        if constexpr (std::is_invocable_v<decltype(&wait_type::get_result),
                                          wait_type*>) {
            return static_cast<wait_type*>(this)->get_result();
        }
    }

    template <typename U>
    static wait_type awair_call(U&& future, SystemScheduler* scheduler,
                                system_handle_t handle) {
        if constexpr (std::is_constructible_v<wait_type, U, SystemScheduler*,
                                              system_handle_t>) {
            return wait_type{std::forward<U>(future), scheduler, handle};
        } else if constexpr (std::is_constructible_v<wait_type, U>) {
            return wait_type{std::forward<U>(future)};
        } else {
            static_assert(sizeof(U) == 0,
                          "无法构造 Waiter，请检查构造函数参数");
        }
    }

    static wait_type await_transform(From&& future, SystemScheduler* scheduler,
                                     system_handle_t handle) {
        return awair_call(std::forward<From>(future), scheduler, handle);
    }
    static wait_type await_transform(From& future, SystemScheduler* scheduler,
                                     system_handle_t handle) {
        return awair_call(std::forward<From>(future), scheduler, handle);
    }
    static wait_type await_transform(const From& future, SystemScheduler* scheduler,
                                     system_handle_t handle) {
        return awair_call(std::forward<From>(future), scheduler, handle);
    }
};

template <typename From>
struct MultiAwaitProtocal : AwaitProtocal<From> {
    using from_type = From;
    using wait_type = From::wait_type;
    MultiAwaitProtocal() : remaining_task_count_(1) {}
    uint32_t task_count(SystemScheduler* scheduler) {
        return remaining_task_count_.load();
    }
    bool await_ready() {
        if (remaining_task_count_.load() == 1) {
            remaining_task_count_.fetch_sub(1);
            assert(remaining_task_count_.load() == 0);
            return true;
        } else {
            return false;
        }
    }
    void await_suspend(system_handle_t caller) {
        if (remaining_task_count_.fetch_sub(1) == 1) {
            caller.resume();
        }
    }
    void submit_task(system_handle_t caller, task_t task) {
        remaining_task_count_.fetch_add(1);
        caller.promise().scheduler_->submit_task(
            [task = std::move(task), caller, this]() {
                task();
                if (remaining_task_count_.fetch_sub(1) == 1) {
                    caller.resume();
                }
            });
    }
   private:
    std::atomic_uint32_t remaining_task_count_{1};
};

template <typename... T>
struct Join {
    struct wait_type;
    using wait_base_t = AwaitProtocal<Join>;
    Join(T&&... tasks) : tasks_(std::forward_as_tuple(tasks...)) {}

    struct wait_type : wait_base_t {
        wait_type(Join&& join) : join_(std::move(join)), wait_base_t() {}
        void await_suspend(system_handle_t caller) noexcept {
            [&]<size_t... I>(std::index_sequence<I...>) {
                (
                    [&]() {
                        using Otp =
                            std::decay_t<decltype(std::get<I>(join_.tasks_))>;
                        std::get<I>(join_.wait_objects_) =
                            std::move(Otp::wait_type::await_transform(
                                std::move(std::get<I>(join_.tasks_)),
                                caller.promise().scheduler_, caller));
                        remaning_task_count_.fetch_add(
                            std::get<I>(join_.wait_objects_)
                                .value()
                                .task_count(caller.promise().scheduler_));
                    }(),
                    ...);
                (caller.promise().scheduler_->submit_task([this, caller]() {
                    std::get<I>(join_.wait_objects_).value().task();
                    if (remaning_task_count_.fetch_sub(1) == 1) {
                        caller.resume();
                    }
                }),
                 ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        Join&& get_result() { return std::move(join_); }
        Join join_;
        std::atomic_uint32_t remaning_task_count_{0};
    };
    decltype(auto) values() {
        return [this]<size_t... I>(std::index_sequence<I...>) {
            return std::make_tuple(
                std::get<I>(wait_objects_).value().get_result()...);
        }(std::make_index_sequence<sizeof...(T)>());
    }

    template <typename Waiter>
    auto operator&&(Waiter&& waiter) -> decltype(auto) {
        return [&]<size_t... I>(std::index_sequence<I...>) {
            return Join<T..., Waiter>{std::get<I>(tasks_)...,
                                      std::forward<Waiter>(waiter)};
        }(std::make_index_sequence<sizeof...(T)>());
    }

   private:
    std::tuple<T...> tasks_{};
    std::tuple<std::optional<typename std::decay_t<T>::wait_type>...>
        wait_objects_{};
};
template <typename T>
struct Future final {
    using function_t = std::function<T(void)>;

   public:
    template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
    Future(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   public:
    struct wait_type;
    using wait_base_t = AwaitProtocal<Future>;
    struct wait_type : wait_base_t {
        wait_type(Future&& future)
            : future_(std::move(future)), wait_base_t() {}
        void task() noexcept {
            try {
                future_.value_ = std::move(future_.fn_());
            } catch (...) {
                future_.exception_ = std::current_exception();
            }
        }
        Future&& get_result() { return std::move(future_); }
        Future future_;
    };

   public:
    T& value() {
        if (exception_) std::rethrow_exception(exception_);
        return value_.value();
    }

   public:
    Future(Future&&) = default;
    Future& operator=(Future&&) = default;
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

   private:
    std::optional<T> value_{std::nullopt};
    std::exception_ptr exception_{nullptr};
    function_t fn_{};
};
template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
Future(Fn&&) -> Future<Rtp>;

template <typename... T>
Join(T&&...) -> Join<T...>;

template <typename A, typename B>
auto operator&&(A&& a, B&& b) -> std::enable_if_t<
    std::is_class_v<typename std::decay_t<A>::wait_type> &&
        std::is_class_v<typename std::decay_t<B>::wait_type>,
    Join<A, B>> {
    return Join{std::forward<A>(a), std::forward<B>(b)};
}

template <typename T, typename Fn>
struct AsyncForeach {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    using base_wait_t = MultiAwaitProtocal<AsyncForeach<T, Fn>>;
    struct wait_type final : base_wait_t {
        wait_type(AsyncForeach& foreach, SystemScheduler* scheduler,
                  system_handle_t handle)
            : foreach_(foreach), base_wait_t() {
            auto count = std::distance(foreach_.begin, foreach_.end);
            auto worker_count = scheduler->worker_count();
            auto mod = count % worker_count;
            auto count_per_worker =
                (count + worker_count - 1) / worker_count - (mod != 0);
            for (uint32_t i = 0; i < worker_count; ++i) {
                auto a = foreach_.begin + i * count_per_worker;
                auto b = (i == worker_count - 1) ? foreach_.end
                                                 : a + count_per_worker;
                auto task = [it = a, end = b, this]() mutable {
                    for (; it < end; ++it) {
                        smart_invoke(foreach_.fn, it, it - foreach_.begin);
                    }
                };
                base_wait_t::submit_task(handle, task);
            }
        }
        [[nodiscard("AsyncForeach::get_result 必须被调用")]]
        AsyncForeach& get_result() {
            return foreach_;
        }

       private:
        template <typename _Fn, typename _T, typename _Offset>
        static void smart_invoke(_Fn&& fn, _T&& it, _Offset&& offset) {
            if constexpr (std::is_invocable_v<_Fn, _T, _Offset>) {
                fn(it, offset);
            } else if constexpr (std::is_invocable_v<_Fn, _T>) {
                fn(it);
            } else {
                using Vt = decltype(*it);
                if constexpr (std::is_invocable_v<_Fn, Vt>) {
                    fn(*it);
                } else if constexpr (std::is_invocable_v<_Fn, Vt, _Offset>) {
                    fn(*it, offset);
                } else {
                    static_assert(false, "invalid call");
                }
            }
        }

        AsyncForeach<T, Fn>& foreach_;
    };

   private:
    T begin;
    T end;
    Fn fn;
};

struct Yield final {
    struct YieldWaitable {
        bool await_ready() const noexcept { return false; }
        void await_suspend(system_handle_t handle_) const noexcept {
            handle_.promise().scheduler_->submit_handle(handle_);
        }
        void await_resume() const noexcept {}
    };
    static YieldWaitable yield(Yield, SystemScheduler* scheduler,
                               system_handle_t handle) {
        return YieldWaitable{};
    }
};

}  // namespace xc::ecs
