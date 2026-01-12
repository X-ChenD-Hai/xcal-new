#pragma once
#include <chrono>

namespace xc::ecs::utility {
struct ClockRecord {
    std::chrono::time_point<std::chrono::steady_clock> time;
    void record() { time = std::chrono::steady_clock::now(); }
    double duration() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                             time)
            .count();
    }
    double duration_ms() const {
        return std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - time)
            .count();
    }
    double duration_us() const {
        return std::chrono::duration<double, std::micro>(
                   std::chrono::steady_clock::now() - time)
            .count();
    }
    double duration_ns() const {
        return std::chrono::duration<double, std::nano>(
                   std::chrono::steady_clock::now() - time)
            .count();
    }
};
}