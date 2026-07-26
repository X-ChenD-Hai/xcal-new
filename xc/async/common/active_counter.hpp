#pragma once
#include <atomic>
namespace xc::async {
class ActiveCounter {
   public:
    ActiveCounter() = default;
    ~ActiveCounter() = default;
    size_t increment() {
        return count_.fetch_add(1, std::memory_order_release) + 1;
    }
    size_t decrement() {
        return count_.fetch_sub(1, std::memory_order_release) - 1;
    }
    size_t count() const { return count_.load(std::memory_order_acquire); }

    void reset() { count_.store(0, std::memory_order_release); }

    void lock() { increment(); }
    void unlock() { decrement(); }

   private:
    std::atomic_size_t count_{0};
};
}  // namespace xc::async