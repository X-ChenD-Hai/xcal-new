#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "./task.hpp"  // IWYU pragma: keep
#include "./types.hpp"

namespace xc::ecs {
struct TimerTask {
    TimerTask() = default;
    TimerTask(task_t&& task, time_duration_t delay)
        : until_time(std::chrono::high_resolution_clock::now() + delay),
          task(std::move(task)) {}
    TimerTask(task_t&& task, time_point_t until_time)
        : until_time(until_time), task(std::move(task)) {}
    auto operator<=>(const TimerTask& other) const {
        return until_time <=> other.until_time;
    }
    time_point_t until_time;
    mutable task_t task;
};

class TimerWorker {
   public:
    using publish_callback_t = std::function<void(task_t&&)>;
    TimerWorker(publish_callback_t expired_callback)
        : running_flag_(true), expired_callback_(expired_callback) {
        thread_ = std::jthread(std::bind(&TimerWorker::worker, this));
    }
    ~TimerWorker() {
        running_flag_ = false;
        cv_.notify_all();
    };

    void publish(TimerTask&& task) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            timer_queue_.emplace(std::move(task));
        }
        cv_.notify_one();
    }

   protected:
    void worker() {
        using namespace std::chrono_literals;
        auto lock = std::unique_lock{mtx_};
        lock.unlock();
        while (running_flag_) {
            lock.lock();
            if (!timer_queue_.empty()) {
                last_time_ = timer_queue_.top().until_time;
            } else {
                last_time_ = std::chrono::high_resolution_clock::now() + 100s;
            }
            if (cv_.wait_until(lock, last_time_, [this]() {
                    return !running_flag_ ||
                           (!timer_queue_.empty() &&
                            timer_queue_.top().until_time < last_time_);
                })) {
                if (running_flag_) last_time_ = timer_queue_.top().until_time;
                lock.unlock();
            } else if (timer_queue_.empty()) {
                lock.unlock();
            } else {
                auto task = std::move(timer_queue_.top().task);
                timer_queue_.pop();
                lock.unlock();
                expired_callback_(std::move(task));
            }
        }
    }

   private:
    std::priority_queue<TimerTask, std::vector<TimerTask>,
                        std::greater<TimerTask>>
        timer_queue_{};

    bool running_flag_{};
    std::mutex mtx_{};
    std::condition_variable cv_{};
    std::jthread thread_{};
    time_point_t last_time_{};
    publish_callback_t expired_callback_{};
};
}  // namespace xc::ecs