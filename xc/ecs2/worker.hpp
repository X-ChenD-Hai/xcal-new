#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <print>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

#include "./config.hpp"
#include "./types.hpp"
#include "promise.hpp"
#include "task.hpp"

namespace xc::ecs {

class Worker {
   public:
    using task_list_t = std::deque<task_t>;
    Worker() = default;
    void worker() {
        thread_id_ = std::this_thread::get_id();
        _WORKER_DEBUG("Worker {} start @ {}", worker_id_, thread_id_);
        while (!wait_flag_.test_and_set()) {
            wait_flag_.notify_all();
        }
        wait_flag_.clear();
        while (run_flag_.test()) {
            // _WORKER_DEBUG("Worker {} wait count {}", worker_id_,
            // wait_count_);
            if (wait_count_ > max_wait_count_) {
                if (wait_flag_.test_and_set()) {
                    wait_count_ = 0;
                    continue;
                }
                wait_flag_.notify_all();
                if (queue_flag_.test_and_set()) {
                    --wait_count_;
                    continue;
                }
                if (steal_callback_ && steal_flag_) {
                    _WORKER_DEBUG("Worker {} start steal tasks", worker_id_);
                    steal_callback_(task_queue_);
                }
                auto empty = task_queue_.empty();
                queue_flag_.clear();
                wait_count_ = 1;
                if (empty) {
                    _WORKER_DEBUG("Worker {} wait ", worker_id_);
                    wait_flag_.wait(true);
                    _WORKER_DEBUG("Worker {} wake up", worker_id_);
                } else {
                    wait_flag_.clear();
                }
            }
            task_t task;
            if (queue_flag_.test_and_set()) {
                continue;
            }
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                assert("task bind worker is not this worker" &&
                       (bind_worker(task) == this ||
                        bind_worker(task) == nullptr));
                task_queue_.pop_front();
                wait_count_ = 0;
            } else {
                ++wait_count_;
            }
            queue_flag_.clear();
            if (wait_count_ == 0) {
                _WORKER_DEBUG("worker {} doing tasks , reamin {}", worker_id_,
                              task_count_.load() - 1);
                current_task_start_time_ = std::chrono::steady_clock::now();
                invoke_task(std::move(task));
                --task_count_;
            }
        }
        _WORKER_DEBUG("Worker {} exit @ {}", worker_id_, thread_id_);
    }
    bool try_enqueue_task(task_t& task) {
        // assert(task && "task is invalid");
        if (queue_flag_.test_and_set()) return false;
        task_queue_.push_back(std::move(task));
        ++task_count_;
        queue_flag_.clear();
        if (wait_flag_.test_and_set()) {
            wait_flag_.clear();
            wait_flag_.notify_all();
        }
        wait_flag_.notify_all();
        wait_flag_.clear();
        return true;
    }
    inline bool try_dequeue_task(task_t& task) noexcept {
        if (queue_flag_.test_and_set()) return false;
        if (task_queue_.empty()) {
            queue_flag_.clear();
            return false;
        }
        task = std::move(task_queue_.front());
        task_queue_.pop_front();
        --task_count_;
        queue_flag_.clear();
        return true;
    }
    void stop() {
        run_flag_.clear();
        if (wait_flag_.test_and_set()) {
            wait_flag_.clear();
            wait_flag_.notify_all();
        }
        wait_flag_.clear();
    }
    void join() { thread_.join(); }
    bool joinable() const { return thread_.joinable(); }
    void start(uint32_t worker_id) {
        worker_id_ = worker_id;
        run_flag_.test_and_set();
        thread_ = std::jthread{&Worker::worker, this};
    }
    inline uint32_t worker_id() const noexcept { return worker_id_; }
    inline std::thread::id thread_id() const noexcept { return thread_id_; }
    inline uint32_t task_count() const noexcept { return task_count_.load(); }
    inline void set_steal_callback(
        std::function<void(task_list_t&)> wait_notify) noexcept {
        steal_callback_ = wait_notify;
    }
    inline bool waiting() const noexcept { return wait_flag_.test(); }
    inline uint32_t wait_count() const noexcept { return wait_count_; }
    inline uint32_t max_wait_count() const noexcept { return max_wait_count_; }
    inline void set_max_wait_count(uint32_t max_wait_count) noexcept {
        max_wait_count_ = max_wait_count;
    }
    inline auto current_task_duration_us() const noexcept {
        return wait_count_
                   ? 0
                   : std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now() -
                         current_task_start_time_)
                         .count();
    }
    inline void enable_steal() noexcept { steal_flag_ = true; }
    inline void disable_steal() noexcept { steal_flag_ = false; }
    inline bool steal_enabled() const noexcept { return steal_flag_; }
    size_t steal(task_list_t& target) {
        size_t c{0};
        auto count = task_count() / 2;
        std::vector<task_t> tmp{};
        if (queue_flag_.test_and_set()) return 0;
        for (uint32_t i = 0; i < count; ++i) {
            if (bind_worker(task_queue_.back()) == this) {
                tmp.push_back(std::move(task_queue_.back()));
            } else {
                target.push_front(std::move(task_queue_.back()));
                ++c;
            }
            task_queue_.pop_back();
        }
        for (auto& t : std::ranges::reverse_view{tmp})
            task_queue_.push_back(std::move(t));
        queue_flag_.clear();
        return c;
    }

   private:
    std::jthread thread_{};
    uint32_t wait_count_{1};
    uint32_t max_wait_count_{1000};
    std::thread::id thread_id_{};
    task_list_t task_queue_{};
    std::atomic_uint32_t task_count_{0};
    bool steal_flag_{};
    uint32_t worker_id_{0};
    std::function<void(task_list_t&)> steal_callback_{};
    std::chrono::steady_clock::time_point current_task_start_time_{};
    alignas(64) std::atomic_flag run_flag_{};
    alignas(64) std::atomic_flag wait_flag_{};
    alignas(64) std::atomic_flag queue_flag_{};
};
}  // namespace xc::ecs
