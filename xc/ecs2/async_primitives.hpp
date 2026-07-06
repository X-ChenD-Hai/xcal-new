#pragma once
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <print>
#include <thread>
#include <type_traits>

#include "./scheduler.hpp"
#include "./types.hpp"
#include "system.hpp"
#include "xc/ecs2/worker.hpp"

namespace xc::ecs {

template <typename... T>
struct Join final {
    struct wait_type;
    Join(T&&... tasks) : tasks_(std::forward_as_tuple(tasks...)) {}

    struct wait_type {
        wait_type(Join&& join, SystemScheduler* scheduler,
                  system_handle_t handle)
            : join_(std::move(join)) {
            handle.promise().begin_wait();
            [&]<size_t... I>(std::index_sequence<I...>) {
                (
                    [&]() {
                        // using Otp =
                        // std::decay_t<decltype(std::get<I>(join_.tasks_))>;
                        // using Owp = Otp::wait_type;
                        std::get<I>(join_.wait_objects_)
                            .emplace(std::move(std::get<I>(join_.tasks_)),
                                     scheduler, handle);
                    }(),
                    ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        bool await_ready() const { return false; }
        void await_suspend(system_handle_t handle) {
            [&]<size_t... I>(std::index_sequence<I...>) {
                (
                    [&]() {
                        auto& o = std::get<I>(join_.wait_objects_).value();
                        if (o.await_ready()) {
                            std::println("await_ready {} {}", I, (void*)&o);
                            handle.promise().end_wait();
                        } else {
                            std::println("await_suspend {} {}", I, (void*)&o);
                            o.await_suspend(handle);
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<sizeof...(T)>());

            handle.promise().end_wait();
        }
        decltype(auto) await_resume() const {
            return const_cast<Join&&>(std::move(join_)).values();
        }
        Join join_;
    };

    decltype(auto) values() && {
        return [this]<size_t... I>(std::index_sequence<I...>) {
            return std::make_tuple([&]() {
                using Tp =
                    decltype(std::get<I>(wait_objects_).value().await_resume());
                if constexpr (std::is_void_v<Tp>) {
                    std::get<I>(wait_objects_).value().await_resume();
                    return std::nullopt;
                } else {
                    return std::get<I>(wait_objects_).value().await_resume();
                }
            }()...);
        }(std::make_index_sequence<sizeof...(T)>());
    }

    template <typename U>
    auto operator&&(U&& waiter) -> Join<T..., U> {
        return [&]<size_t... I>(std::index_sequence<I...>) {
            return Join<T..., U>{std::get<I>(tasks_)...,
                                 std::forward<U>(waiter)};
        }(std::make_index_sequence<sizeof...(T)>());
    }

   private:
    std::tuple<const T&...> tasks_{};
    std::tuple<std::optional<typename std::decay_t<T>::wait_type>...>
        wait_objects_{};
};
template <typename T>
static constexpr bool is_async_join_onject = false;
template <typename... T>
static constexpr bool is_async_join_onject<Join<T...>> = true;

template <typename T, bool = false>
struct Future final {
    using function_t = std::function<T(void)>;

   public:
    template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
    Future(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   public:
    struct wait_type {
        wait_type(Future&& future, SystemScheduler* scheduler,
                  system_handle_t handle)
            : future_(std::move(future)) {
            handle.promise().begin_wait();
            handle.promise().submit_task([this]() {
                try {
                    future_.value_ = std::move(future_.fn_());
                } catch (...) {
                    future_.exception_ = std::current_exception();
                }
            });
        }
        bool await_ready() { return false; }
        void await_suspend(system_handle_t handle) {
            handle.promise().end_wait();
        }

        T&& await_resume() { return std::move(future_).value(); }
        Future future_;
    };

   public:
    T&& value() && {
        if (exception_) std::rethrow_exception(exception_);
        return std::move(value_).value();
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
template <typename T>
class FuturePromise;
template <typename T>
class FutureWait;
template <typename T>
using future_handle_t = std::coroutine_handle<FuturePromise<T>>;
template <typename T>
class Future<T, false> final {
   public:
    friend FuturePromise<T>;
    friend FutureWait<T>;
    using promise_type = FuturePromise<T>;
    using wait_type = FutureWait<T>;

    future_handle_t<T> handle_;
};

template <typename T>
class FutureWait {
   public:
    FutureWait(const FutureWait&) = delete;
    FutureWait& operator=(const FutureWait&) = delete;
    template <IsPromise H>
    using handle_t = std::coroutine_handle<H>;
    template <IsPromise H>
    FutureWait(Future<T>&& future, SystemScheduler* scheduler,
               handle_t<H> handle)
        : future_(std::move(future)) {
        future_.handle_.promise().set_scheduler(scheduler);
        handle.promise().begin_wait();
    }
    bool await_ready() { return future_.handle_.done(); }
    template <IsPromise P>
    void await_suspend(handle_t<P> handle) {
        std::println("----------== future {} suspend",
                     future_.handle_.address());
        handle.promise().submit_task(future_.handle_);
        handle.promise().end_wait();
    }
    T&& await_resume() {
        std::println("-------== future {} resume", future_.handle_.address());
        return future_.handle_.promise().value();
    }
    ~FutureWait() {
        if (future_.handle_) {
            std::println("--------== destroy future {}",
                         future_.handle_.address());
            assert("future is not done" && future_.handle_.done());
            future_.handle_.destroy();
        }
    }
    Future<T, false> future_{nullptr};
};
template <typename T>
class FuturePromise : public Promise<FuturePromise<T>> {
   public:
    Future<T> get_return_object() {
        return Future<T, false>{
            this->handle_ =
                std::coroutine_handle<FuturePromise>::from_promise(*this)};
    }
    void return_value(T&& v) { value_ = std::forward<T>(v); }
    T&& value() {
        if (this->exception_) std::rethrow_exception(this->exception_);
        return std::move(value_).value();
    }
    bool done() { return this->exception_ || (value_ != std::nullopt); }

   private:
    std::optional<T> value_;
};

template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
Future(Fn&&) -> Future<Rtp, true>;

template <typename... T>
Join(T&&...) -> Join<T...>;

template <typename A, typename B>
    requires(std::is_class_v<typename std::decay_t<A>::wait_type> &&
             std::is_class_v<typename std::decay_t<B>::wait_type> &&
             !is_async_join_onject<A> && !is_async_join_onject<B>)
auto operator&&(A&& a, B&& b) -> decltype(auto) {
    return Join{std::forward<A>(a), std::forward<B>(b)};
}

template <typename T, typename Fn>
struct AsyncForeach {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    struct wait_type final {
        wait_type(wait_type&& o) : foreach_(std::move(o.foreach_)) {}
        template <typename U>
        wait_type(U&& foreach, SystemScheduler* scheduler,
                  system_handle_t handle)
            : foreach_(std::forward<U>(foreach)) {
            handle.promise().begin_wait();
            const auto count = std::distance(foreach_.begin, foreach_.end);
            const auto worker_count = scheduler->worker_count();
            const auto mod = count % worker_count;
            const auto count_per_worker =
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
                handle.promise().submit_task(task);
            }
        }
        void await_resume() {}
        bool await_ready() const { return false; }
        void await_suspend(system_handle_t handle) {
            handle.promise().end_wait();
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

        const AsyncForeach<T, Fn> foreach_;
    };

   private:
    const T begin;
    const T end;
    const Fn fn;
};

struct Yield final {
    struct wait_type {
        wait_type(Yield&& _, SystemScheduler* scheduler,
                  system_handle_t handle) {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(system_handle_t handle_) const noexcept {
            handle_.promise().resubmit();
        }
        void await_resume() const noexcept {}
    };
};

struct CurrentWorker {
    struct wait_type {
        wait_type(CurrentWorker&& _, SystemScheduler* scheduler,
                  system_handle_t handle)
            : ptr(scheduler->current_worker()) {}
        constexpr bool await_ready() const noexcept { return true; }
        void await_suspend(system_handle_t handle) const noexcept {}
        const Worker* await_resume() const noexcept { return ptr; }
        const Worker* ptr;
    };
};
struct BindWorker {
    struct wait_type {
        wait_type(BindWorker&& o, SystemScheduler* scheduler,
                  system_handle_t handle)
            : bind(o.bind) {
            handle.promise().bind(o.bind);
        }
        constexpr bool await_ready() const noexcept {
            return std::this_thread::get_id() == bind->thread_id();
        }
        void await_suspend(system_handle_t handle_) const noexcept {
            handle_.promise().resubmit();
        }
        void await_resume() const noexcept {}
        const Worker* bind;
    };
    BindWorker(const Worker* bind) : bind(bind) {}
    const Worker* bind;
};
struct DispatchTo {
    struct wait_type {
        wait_type(DispatchTo&& o, SystemScheduler* scheduler,
                  system_handle_t handle)
            : last_bind(handle.promise().bind_worker()), handle(handle) {
            handle.promise().bind(o.dispatch);
        }
        constexpr bool await_ready() const noexcept {
            return std::this_thread::get_id() ==
                   handle.promise().bind_worker()->thread_id();
        }
        void await_suspend(system_handle_t handle_) const noexcept {
            handle_.promise().resubmit();
        }
        void await_resume() const noexcept { handle.promise().bind(last_bind); }
        const Worker* last_bind;
        const system_handle_t handle;
    };
    DispatchTo(const Worker* dispatch) : dispatch(dispatch) {}
    const Worker* dispatch;
};

}  // namespace xc::ecs
