#pragma once
#include <atomic>
namespace xc::async {
class AsyncGate {
   public:
    AsyncGate() { start(); };
    ~AsyncGate() { close(); };
    static constexpr uint8_t CLOSED = 0;
    static constexpr uint8_t CLOSING = 1;
    static constexpr uint8_t RUNNING = 2;

    bool closed() const {
        return state_.load(std::memory_order_acquire) == CLOSED;
    }
    bool closing() const {
        return state_.load(std::memory_order_acquire) == CLOSING;
    }
    bool running() const {
        return state_.load(std::memory_order_acquire) == RUNNING;
    }

    template <typename Fn>
    bool entry_close(Fn&& fn) {
        if (!begin_close()) return false;
        fn();
        end_close();
        return true;
    }
    void close() {
        entry_close([]() {});
    }
    bool start() {
        auto expected = state_.load(std::memory_order_acquire);
        do {
            if (expected == RUNNING)
                return true;
            else if (expected == CLOSING)
                return false;
            expected = CLOSED;
        } while (!state_.compare_exchange_strong(expected, RUNNING,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire));
        return true;
    }

    bool begin_close() {
        auto expected = state_.load(std::memory_order_acquire);
        if (expected != RUNNING) return false;
        return state_.compare_exchange_strong(expected, CLOSING,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire);
    }
    bool end_close() {
        auto expected = state_.load(std::memory_order_acquire);
        if (expected != CLOSING) return false;
        return state_.compare_exchange_strong(expected, CLOSED,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire);
    }

   private:
    std::atomic_uint8_t state_{RUNNING};
};
}  // namespace xc::async