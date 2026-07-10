#pragma once
#include <malloc.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "./scheduler.hpp"
#include "./task.hpp"
#include "./types.hpp"
#include "config.hpp"
#include "promise.hpp"
#include "structure/ring_buffer.hpp"
#include "system.hpp"
#include "worker.hpp"

namespace xc::ecs {
template <typename Derived>
struct MoveAsWaitable {
    template <IsPromise P>
    auto get_awaitable(SystemScheduler* scheduler,
                       std::coroutine_handle<P> handle) {
        return typename Derived::wait_type{
            std::move(*static_cast<Derived*>(this)), scheduler, handle};
    }
};
template <typename... T>
struct Join final : public MoveAsWaitable<Join<T...>> {
    using tuple_t = std::tuple<std::remove_reference_t<T>...>;

    template <typename... Args>
    Join(Args&&... tasks) : tasks_(std::move(tasks)...) {}

    struct wait_type {
        template <IsPromise P>
        wait_type(Join&& join, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : join_(std::move(join)) {
            [&]<size_t... I>(std::index_sequence<I...>) {
                (
                    [&]() {
                        std::get<I>(join_.wait_objects_)
                            .emplace(std::move(std::get<I>(join_.tasks_)),
                                     scheduler, handle);
                    }(),
                    ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        bool await_ready() const {
            return [&]<size_t... I>(std::index_sequence<I...>) {
                return ([&]() {
                    return join_.ready_[I] = std::get<I>(join_.wait_objects_)
                                                 .value()
                                                 .await_ready();
                }() && ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) {
            PromisLockGuard lk(handle.promise());
            [&]<size_t... I>(std::index_sequence<I...>) {
                (
                    [&]() {
                        auto& o = std::get<I>(join_.wait_objects_).value();
                        if (!join_.ready_[I]) {
                            _SCHEDULER_DEBUG("await_suspend {} {}", I,
                                             (void*)&o);
                            o.await_suspend(handle);
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        decltype(auto) await_resume() const {
            return const_cast<Join&&>(std::move(join_)).values();
        }
        Join&& join_;
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
    tuple_t tasks_{};
    std::tuple<std::optional<typename std::decay_t<T>::wait_type>...>
        wait_objects_{};
    std::array<bool, sizeof...(T)> ready_{};
};
template <typename... T>
Join(T&&...) -> Join<T...>;
template <typename T>
static constexpr bool is_async_join_onject = false;
template <typename... T>
static constexpr bool is_async_join_onject<Join<T...>> = true;

template <typename A, typename B>
    requires(std::is_class_v<typename std::decay_t<A>::wait_type> &&
             std::is_class_v<typename std::decay_t<B>::wait_type> &&
             !is_async_join_onject<A> && !is_async_join_onject<B>)
auto operator&&(A&& a, B&& b) -> decltype(auto) {
    return Join{std::forward<A>(a), std::forward<B>(b)};
}

template <typename T, bool = false>
struct Future;
template <typename T>
struct Future<T, true> final : public MoveAsWaitable<Future<T, true>> {
    using function_t = std::function<T(void)>;

   public:
    template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
    Future(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   public:
    struct wait_type {
        template <IsPromise P>
        wait_type(Future&& future, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : future_(std::move(future)) {}
        bool await_ready() { return false; }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) {
            assert(future_.fn_ && "future fn is null");
            handle.promise().submit_task(FuncTask{[this]() {
                try {
                    assert(future_.fn_ && "future fn is null");
                    future_.value_ = future_.fn_();
                } catch (...) {
                    future_.exception_ = std::current_exception();
                }
            }});
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
class Future<T, false> final : public MoveAsWaitable<Future<T, false>> {
   public:
    friend FuturePromise<T>;
    friend FutureWait<T>;
    using promise_type = FuturePromise<T>;
    using wait_type = FutureWait<T>;
    Future() = default;
    Future(future_handle_t<T> handle, std::shared_ptr<std::optional<T>> v)
        : state_(handle.promise().state()), value_(v) {
        assert(state_ && "future state is null");
    }
    T& value() {
        assert(value_ && "value is null");
        return value_->value();
    }
    ~Future() {
        if (state_) {
            assert("future is submitted but not done" &&
                   (state_->done() || state_->remain_task_count.load(
                                          std::memory_order_acquire) == 0));
        }
    }

   private:
    std::shared_ptr<PromiseState> state_{nullptr};
    std::shared_ptr<std::optional<T>> value_{
        std::make_shared<std::optional<T>>()};
};
template <typename T>
class FutureWait {
   public:
    FutureWait(const FutureWait&) = delete;
    FutureWait& operator=(const FutureWait&) = delete;
    FutureWait(FutureWait&& o) noexcept { std::swap(o.future_, future_); }
    template <IsPromise H>
    using handle_t = std::coroutine_handle<H>;
    template <IsPromise H>
    FutureWait(Future<T>&& future, SystemScheduler* scheduler,
               handle_t<H> handle)
        : future_(std::move(future)) {}
    bool await_ready() { return false; }
    template <IsPromise P>
    void await_suspend(handle_t<P> handle) {
        _SCHEDULER_DEBUG("future {} suspend",
                         (void*)&future_.handle_.promise());
        assert(future_.state_ && future_.state_->promise &&
               "future state is null or promise is null");
        handle.promise().add_child(*future_.state_->promise);
    }
    T&& await_resume() {
        _SCHEDULER_DEBUG("future {} resume", (void*)&future_.handle_.promise());
        assert("future is not done" && future_.state_->promise == nullptr);
        return std::move(future_.value());
    }
    ~FutureWait() = default;
    Future<T, false> future_{};
};
template <typename T>
class FuturePromise : public Promise<FuturePromise<T>> {
    using Super = Promise<FuturePromise<T>>;

   public:
    Future<T> get_return_object() {
        _SCHEDULER_DEBUG("future get_return_object {}", (void*)this);
        auto v{std::make_shared<std::optional<T>>()};
        value_ = v;
        return Future<T, false>{Super::init_handle(), v};
    }
    void return_value(T&& v) {
        if (value_.use_count()) {
            *value_.lock() = std::move(v);
        }
    }
    ~FuturePromise() {}

   private:
    std::weak_ptr<std::optional<T>> value_{};
};

template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
Future(Fn&&) -> Future<Rtp, true>;

template <typename T, typename Fn>
struct AsyncForeach : public MoveAsWaitable<AsyncForeach<T, Fn>> {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    struct wait_type final {
        template <typename U, IsPromise P>
        wait_type(U&& foreach, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : foreach_(std::forward<U>(foreach)) {}
        void await_resume() {}
        bool await_ready() const { return false; }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) {
            const auto count = std::distance(foreach_.begin, foreach_.end);
            const auto worker_count =
                handle.promise().scheduler()->worker_count();
            const auto mod = count % worker_count;
            const auto count_per_worker =
                (count + worker_count - 1) / worker_count - (mod != 0);
            handle.promise().begin_wait();
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
    bool await_ready() const noexcept { return false; }
    template <IsPromise P>
    void await_suspend(std::coroutine_handle<P> handle_) const noexcept {
        handle_.promise().resubmit();
    }
    void await_resume() const noexcept {}
};

struct CurrentWorker : public MoveAsWaitable<CurrentWorker> {
    struct wait_type {
        template <IsPromise P>
        wait_type(CurrentWorker&& _, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : ptr(scheduler->current_worker()) {}
        constexpr bool await_ready() const noexcept { return true; }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) const noexcept {
            assert(false && "await_suspend not implemented");
        }
        const Worker* await_resume() const noexcept { return ptr; }
        const Worker* ptr;
    };
};
struct BindWorker : public MoveAsWaitable<BindWorker> {
    struct wait_type {
        template <IsPromise P>
        wait_type(BindWorker&& o, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : bind(o.bind) {
            handle.promise().bind(o.bind);
        }
        bool await_ready() const noexcept {
            return std::this_thread::get_id() == bind->thread_id();
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle_) const noexcept {
            handle_.promise().resubmit();
        }
        void await_resume() const noexcept {}
        const Worker* bind;
    };
    BindWorker(const Worker* bind) : bind(bind) {}
    const Worker* bind{nullptr};
};
struct DispatchTo : public MoveAsWaitable<DispatchTo> {
    struct wait_type {
        template <IsPromise P>
        wait_type(DispatchTo&& o, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle)
            : last_bind(handle.promise().bind_worker()),
              primise(&handle.promise()) {
            handle.promise().bind(o.dispatch);
        }
        bool await_ready() const noexcept {
            return std::this_thread::get_id() ==
                   primise->bind_worker()->thread_id();
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle_) const noexcept {
            handle_.promise().resubmit();
        }
        void await_resume() const noexcept { primise->bind(last_bind); }
        const Worker* last_bind;
        BasePromise* const primise;
    };
    DispatchTo(const Worker* dispatch) : dispatch(dispatch) {}
    const Worker* dispatch{nullptr};
};
struct Sleep {
    Sleep(time_duration_t duration)
        : until(time_point_t::clock::now() + duration) {}
    Sleep(time_point_t until_time) : until(until_time) {}

    constexpr bool await_ready() const noexcept {
        return time_point_t::clock::now() >= until;
    }
    template <IsPromise P>
    void await_suspend(std::coroutine_handle<P> handle_) const noexcept {
        handle_.promise().begin_wait();
        handle_.promise().submit_timeout_task(
            AsyncEndWaitTask{&handle_.promise()}, until);
    }

    void await_resume() const noexcept {}
    time_point_t until;
};
template <typename T>
using resume_type = std::invoke_result_t<decltype(&T::await_resume), T*>;

template <typename T>
struct WhenAll : public MoveAsWaitable<WhenAll<T>> {
    using wait_value_t = T::wait_type;
    using resume_value_t = resume_type<wait_value_t>;
    struct wait_type {
        template <IsPromise P>
        wait_type(WhenAll&& o, SystemScheduler* scheduler,
                  std::coroutine_handle<P> handle) {
            for (auto& v : o.view) {
                wait_objects_.emplace_back(std::move(v), scheduler, handle);
            }
        }
        constexpr bool await_ready() noexcept {
            for (auto&& [i, v] : wait_objects_ | std::views::enumerate) {
                if (!v.await_ready()) {
                    unready_indices.push_back(i);
                }
            }
            return unready_indices.empty();
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            PromisLockGuard lk(handle.promise());
            for (auto i : unready_indices) {
                wait_objects_[i].await_suspend(handle);
            }
        }
        decltype(auto) await_resume() noexcept {
            return wait_objects_ | std::views::transform([](auto&& v) {
                       return v.await_resume();
                   }) |
                   std::ranges::to<std::vector>();
        }
        std::vector<wait_value_t> wait_objects_{};
        std::vector<size_t> unready_indices{};
    };

    WhenAll(std::vector<T>& view) : view(std::move(view)) {}
    WhenAll(std::vector<T>&& view) : view(std::move(view)) {}

    std::vector<T> view{};
};

template <typename T, size_t N>
class Channel {
   public:
    Channel(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel& operator=(Channel&&) = delete;

   public:
    struct RecvWait {
       public:
        RecvWait(const RecvWait&) = delete;
        RecvWait& operator=(const RecvWait&) = delete;
        RecvWait& operator=(RecvWait&&) = delete;
        RecvWait(RecvWait&& o) : channel(o.channel) {
            std::swap(v, o.v);
            std::swap(promise, o.promise);
        }

       public:
        RecvWait(Channel& ch) : channel(ch) {}
        constexpr bool await_ready() noexcept {
            v = channel.try_recv();
            return v.has_value();
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            promise = &handle.promise();
            channel.add_recv(this);
        }
        T&& await_resume() noexcept {
            channel.remain_msg_.fetch_sub(1, std::memory_order_release);
            return std::move(v.value());
        }
        ~RecvWait() {}
        Channel& channel;
        std::optional<T> v{};
        BasePromise* promise{nullptr};
    };
    struct SendWait {
       public:
        SendWait(const SendWait&) = delete;
        SendWait& operator=(const SendWait&) = delete;
        SendWait& operator=(SendWait&&) = delete;
        SendWait(SendWait&& o) : channel(o.channel) {
            std::swap(value, o.value);
            std::swap(promise, o.promise);
        }

       public:
        SendWait(Channel& ch, T&& value);
        constexpr bool await_ready() noexcept {
            return channel.try_send(value);
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            promise = &handle.promise();
            channel.add_send(this);
        }
        void await_resume() noexcept {}
        Channel& channel;
        BasePromise* promise{nullptr};
        T value;
    };

    Channel() {
        recv_flag_.clear();
        send_flag_.clear();
    }

    RecvWait recv() { return RecvWait(*this); }
    template <typename... Args>
    SendWait send(Args&&... args) {
        return SendWait(*this, std::forward<Args>(args)...);
    }

   protected:
    void add_recv(RecvWait* p) {
        if (!p) return;
        p->promise->begin_wait();
        while (recv_flag_.test_and_set());
        recv_promises_.push_back(p);
        recv_flag_.clear();
        notify_sender();
    }
    void add_send(SendWait* p) {
        if (!p) return;
        p->promise->begin_wait();
        while (send_flag_.test_and_set());
        send_promises_.push_back(p);
        send_flag_.clear();
        notify_recv();
    }

    std::optional<T> try_recv() {
        T tmp;
        if (buffer_.try_dequeue(tmp)) {
            notify_sender();
            return tmp;
        }
        return std::nullopt;
    }
    bool try_send(T& value) {
        if (buffer_.try_enqueue(value)) {
            notify_recv();
            return true;
        }
        return false;
    }
    void notify_recv() {
        while (recv_flag_.test_and_set());
        BasePromise* p{nullptr};
        if (!recv_promises_.empty()) {
            auto v = try_recv();
            if (v.has_value()) {
                recv_promises_.front()->v = std::move(v.value());
                p = recv_promises_.front()->promise;
                recv_promises_.pop_front();
            }
        }
        recv_flag_.clear();
        if (p) p->async_end_wait();
    }
    void notify_sender() {
        while (send_flag_.test_and_set());
        BasePromise* p{nullptr};
        if (!send_promises_.empty()) {
            SendWait* s = send_promises_.front();
            if (try_send(s->value)) {
                p = s->promise;
                send_promises_.pop_front();
            }
        }
        send_flag_.clear();
        if (p) p->async_end_wait();
    }

   private:
    alignas(64) std::atomic_flag recv_flag_{};
    alignas(64) std::atomic_flag send_flag_{};
    structure::RingBuffer<T, N> buffer_{};
    std::deque<RecvWait*> recv_promises_{};
    std::deque<SendWait*> send_promises_{};
    std::atomic_uint32_t remain_msg_{0};
};
template <typename T, size_t N>
inline Channel<T, N>::SendWait::SendWait(Channel& ch, T&& value)
    : channel(ch), value(std::move(value)) {}

}  // namespace xc::ecs
