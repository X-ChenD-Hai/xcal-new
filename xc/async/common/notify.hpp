#pragma once
#include <atomic>
namespace xc::async {
template <typename T, typename Notifier>
class alignas(64) NotifyOnce {
   public:
    NotifyOnce(const NotifyOnce&) = delete;
    NotifyOnce& operator=(const NotifyOnce&) = delete;

   public:
    NotifyOnce() : target_(nullptr) {}
    NotifyOnce(T* tracker) : target_(tracker) {}
    NotifyOnce(NotifyOnce&& o) : target_(o.take()) {}
    NotifyOnce& operator=(NotifyOnce&& o) {
        notify(o.take());
        return *this;
    }

    ~NotifyOnce() { notify(); }
    auto notify(T* replace = nullptr) {
        return Notifier::notify(
            target_.exchange(replace, std::memory_order_acq_rel));
    }
    T* take() { return target_.exchange(nullptr, std::memory_order_acq_rel); }
    void store(T* replace) {
        target_.store(replace, std::memory_order_release);
    }

    T* load() { return target_.load(std::memory_order_acquire); }
    const T* load() const { return target_.load(std::memory_order_acquire); }
    T* operator->() { return load(); }
    const T* operator->() const { return load(); }

   private:
    std::atomic<T*> target_{nullptr};
};

}  // namespace xc::async