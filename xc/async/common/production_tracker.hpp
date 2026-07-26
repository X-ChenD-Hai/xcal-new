#pragma once
#include <future>
#include <optional>
#include <type_traits>

#include "xc/async/common/active_counter.hpp"
#include "xc/async/common/async_gate.hpp"

namespace xc::async {
template <typename T, typename Derived = void>
class ProductionTracker;
template <typename T>
class ProducerGuard {
    friend typename T::tracker_type;
    ProducerGuard(const ProducerGuard&) = delete;
    ProducerGuard& operator=(const ProducerGuard&) = delete;

    ProducerGuard(T* tracker);

   public:
    ProducerGuard(ProducerGuard&& o) : tracker_(o.tracker_) {
        o.tracker_ = nullptr;
    }
    ProducerGuard& operator=(ProducerGuard&& o) {
        complete();
        tracker_ = o.tracker_;
        o.tracker_ = nullptr;
        return *this;
    };
    ~ProducerGuard();
    operator bool() const noexcept { return producible(); }
    bool producible() const noexcept { return tracker_; }

    void complete();

   private:
    T* tracker_{nullptr};
};
template <typename T>
class ProductionTrackerImpl {
   public:
    ProductionTrackerImpl(T&& on_closed)
        : on_closed_(std::forward<T>(on_closed)) {}

    void set_close_callback(T&& on_closed) {
        on_closed_ = std::forward<T>(on_closed);
    }

   protected:
    void on_close() { on_closed_(); }
    T on_closed_{};
};
template <>
class ProductionTrackerImpl<void> {
   protected:
    void on_close() {}
};

template <typename T, typename Derived>
class alignas(64) ProductionTracker : public ProductionTrackerImpl<T> {
    using self_t = ProductionTracker<T, Derived>;
    using derived_t =
        std::conditional_t<std::is_void_v<Derived>, self_t, Derived>;
    using tracker_type = self_t;
    friend ProducerGuard<derived_t>;

   public:
    using ProductionTrackerImpl<T>::ProductionTrackerImpl;

    ProductionTracker() = default;
    ~ProductionTracker() = default;

    ProducerGuard<derived_t> spawn() {
        return ProducerGuard<derived_t>(static_cast<derived_t*>(this));
    }
    bool running() const { return gate_.running(); }
    bool closing() const noexcept { return gate_.closing(); }
    bool closed() const { return gate_.closed(); }
    template <typename U = Derived>
    void start() {
        gate_.start();
    }

    void close() {
        if (!gate_.begin_close()) return;
        if (active_counter_.count() == 0) {
            try_final_close();
        }
    }

   protected:
    using ProductionTrackerImpl<T>::on_close;

    bool try_final_close() {
        if (!gate_.end_close()) return false;
        static_cast<derived_t*>(this)->on_close();
        return true;
    }

   private:
    ActiveCounter active_counter_{};
    AsyncGate gate_{};
};

template <typename T>
class SyncProductionTracker
    : public ProductionTracker<T, SyncProductionTracker<T>> {
    using base_t = ProductionTracker<T, SyncProductionTracker<T>>;

   public:
    template <typename... Args>
    SyncProductionTracker(Args&&... args)
        : base_t(std::forward<Args>(args)...) {
        close_future_ = close_promise_->get_future();
    }
    void start() {
        close_promise_ = std::promise<void>{};
        close_future_ = close_promise_->get_future();
        base_t::start();
    }

    void on_close() {
        base_t::on_close();
        close_promise_->set_value();
    }

    void sync_close() {
        base_t::close();
        wait_until_closed();
    }
    void wait_until_closed() {
        if (base_t::closed()) return;
        close_future_->wait();
    }

   public:
    std::optional<std::promise<void>> close_promise_{std::promise<void>{}};
    std::optional<std::future<void>> close_future_{};
};

template <typename T>
ProductionTracker(T&& on_closed) -> ProductionTracker<T>;
ProductionTracker() -> ProductionTracker<void>;
template <typename T>
SyncProductionTracker(T&& on_closed) -> SyncProductionTracker<T>;
SyncProductionTracker() -> SyncProductionTracker<void>;

template <typename T>
inline ProducerGuard<T>::ProducerGuard(T* tracker) : tracker_(tracker) {
    tracker_->active_counter_.increment();
    if (!tracker_->gate_.running()) complete();
}
template <typename T>
inline void ProducerGuard<T>::complete() {
    if (!tracker_) return;
    auto remain = tracker_->active_counter_.decrement();
    if (remain == 0 && tracker_->gate_.closing()) {
        tracker_->try_final_close();
    }
    tracker_ = nullptr;
}
template <typename T>
inline ProducerGuard<T>::~ProducerGuard() {
    complete();
}
}  // namespace xc::async
