#pragma once
#include <atomic>
namespace xc::async {
template <typename T, typename Notifier>
class NotifyOnce {
   public:
    NotifyOnce() : promise_(nullptr) {}
    ~NotifyOnce() { notify(); }
    void notify(T* replace = nullptr) {
        auto expected = promise_.load(std::memory_order_relaxed);
        auto to_resume = expected;
        do {
            to_resume = expected;
        } while (promise_.compare_exchange_strong(expected, replace));
        Notifier::notify(to_resume);
    }

   private:
    std::atomic<T*> promise_{nullptr};
};

}  // namespace xc::async