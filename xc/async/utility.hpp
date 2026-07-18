#pragma once
#include <chrono>

namespace xc::async::utility {
struct ClockRecord {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point time{};
    void record() { time = clock::now(); }
    double duration() const {
        return std::chrono::duration<double>(clock::now() - time).count();
    }
    double duration_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   clock::now() - time)
            .count();
    }
    double duration_us() const {
        return std::chrono::duration<double, std::micro>(clock::now() - time)
            .count();
    }
    double duration_ns() const {
        return std::chrono::duration<double, std::nano>(clock::now() - time)
            .count();
    }
};
}  // namespace xc::async::utility