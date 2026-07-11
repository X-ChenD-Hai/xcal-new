#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "./config.hpp"
#include "./promise.hpp"
#include "./system.hpp"
#include "./task.hpp"
#include "./types.hpp"
#include "./worker.hpp"
#include "timer_woker.hpp"

namespace xc::ecs {

class SystemScheduler {
   public:
    static constexpr size_t StealUntilMaxUs = 500;
    using system_fn_t = System (*)();
    using worker_paload_t = std::tuple<uint32_t, uint32_t, uint32_t>;
    SystemScheduler() = default;
    void start_workers(uint32_t count = std::thread::hardware_concurrency()) {
        _SCHEDULER_DEBUG("Start {} workers", count);
        workers_.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            workers_.emplace_back(std::make_unique<Worker>());
            workers_.back()->set_steal_callback(std::bind(
                &SystemScheduler::work_steal, this, std::placeholders::_1));
            workers_.back()->start(i);
            worker_crefs_.emplace_back(workers_.back().get());
        }
        _SCHEDULER_DEBUG("All workers started");
        timer_worker_ = std::make_unique<TimerWorker>(
            std::bind(&SystemScheduler::submit_expired_task, this,
                      std::placeholders::_1));
    }
    void add_system(System&& system) {
        auto sys = std::make_unique<System>(std::move(system));
        sys->state_->promise->set_scheduler(this);
        submit_task(ResumeUntilOnceTask{sys->state_->promise});
        systems_instencees_.emplace_back(std::move(sys));
    }
    void stop_workers() {
        _SCHEDULER_DEBUG("Stop all workers");
        destroy_workers();
        _SCHEDULER_DEBUG("All workers stopped");
    }
    bool try_submit_task(task_t& task) {
        _SCHEDULER_DEBUG("try submit {}", task_type(task));
        auto bind = bind_worker(task);
        assert("bind worker is not nullptr or not in workers" &&
               (bind == nullptr ||
                std::count_if(workers_.begin(), workers_.end(),
                              [bind](auto& w) { return w.get() == bind; })) >
                   0);
        if (bind) {
            _SCHEDULER_DEBUG("submit task to worker {} which is bind",
                             bind->worker_id());
            return const_cast<Worker*>(bind)->try_enqueue_task(task);
        }
        for (auto& worker : workers_) {
            if (!worker->waiting()) continue;
            if (worker->try_enqueue_task(task)) {
                _SCHEDULER_DEBUG("submit task to worker {} which is waiting",
                                 worker->worker_id());
                return true;
            }
        }
        auto worker_paloads = this->worker_paloads();
        auto min_worker =
            std::min_element(workers_.begin(), workers_.end(),
                             [&](const auto& a, const auto& b) {
                                 return worker_paloads[a->worker_id()] <
                                        worker_paloads[b->worker_id()];
                             });

        if (min_worker != workers_.end() &&
            (*min_worker)->try_enqueue_task(task)) {
            _SCHEDULER_DEBUG("submit task to worker {} which is spin waiting",
                             (*min_worker)->worker_id());
            return true;
        }
        if (rand_worker().try_enqueue_task(task)) {
            _SCHEDULER_DEBUG(
                "submit task to worker {} which is doing other tasks",
                rand_worker().worker_id());
            return true;
        }
        return false;
    }
    bool try_submit_prior_task(task_t& task) {
        _SCHEDULER_DEBUG("try submit prior {}", task_type(task));
        auto bind = bind_worker(task);
        assert("bind worker is not nullptr or not in workers" &&
               (bind == nullptr ||
                std::count_if(workers_.begin(), workers_.end(),
                              [bind](auto& w) { return w.get() == bind; })) >
                   0);
        if (bind) {
            _SCHEDULER_DEBUG("submit prior task to worker {} which is bind",
                             bind->worker_id());
            return const_cast<Worker*>(bind)->try_enqueue_prior_task(task);
        }
        for (auto& worker : workers_) {
            if (!worker->waiting()) continue;
            if (worker->try_enqueue_prior_task(task)) {
                _SCHEDULER_DEBUG(
                    "submit prior task to worker {} which is waiting",
                    worker->worker_id());
                return true;
            }
        }
        auto worker_paloads = this->worker_paloads();
        auto min_worker =
            std::min_element(workers_.begin(), workers_.end(),
                             [&](const auto& a, const auto& b) {
                                 return worker_paloads[a->worker_id()] <
                                        worker_paloads[b->worker_id()];
                             });

        if (min_worker != workers_.end() &&
            (*min_worker)->try_enqueue_prior_task(task)) {
            _SCHEDULER_DEBUG(
                "submit prior task to worker {} which is spin waiting",
                (*min_worker)->worker_id());
            return true;
        }
        if (auto& worker = rand_worker(); worker.try_enqueue_prior_task(task)) {
            _SCHEDULER_DEBUG(
                "submit  prior task to worker {} which is doing other tasks",
                worker.worker_id());
            return true;
        }
        return false;
    }

    void submit_task(task_t&& task) {
        while (!try_submit_task(task)) {
            std::this_thread::yield();
        }
    }
    void submit_prior_task(task_t&& task) {
        while (!try_submit_prior_task(task)) {
            std::this_thread::yield();
        }
    }

    void update() {
        _SCHEDULER_DEBUG("Run scheduler");
        assert(!workers_.empty());
        if (systems_instencees_.empty()) return;
        _SCHEDULER_DEBUG("Enter Loop");
        std::vector<std::exception_ptr> exceptions;
        do {
            {
                std::unique_lock<std::mutex> lock(steal_mutex_);
                // _SCHEDULER_DEBUG("Wait for steal");
                steal_cv_.wait_for(lock, std::chrono::microseconds(100),
                                   [this]() { return steal_flag_; });
                // steal_cv_.wait(lock, [this]() { return steal_flag_; });
                steal_flag_ = false;
                // _SCHEDULER_DEBUG("Steal done");
                lock.unlock();
            }
            flush(&exceptions);
            if (systems_instencees_.empty()) break;
            std::this_thread::yield();
        } while (1);
        systems_instencees_.clear();
        for (auto& exception : exceptions) {
            try {
                std::rethrow_exception(exception);
            } catch (const std::exception& e) {
                _SCHEDULER_DEBUG("Exception: {}", e.what());
            }
        }
        _SCHEDULER_DEBUG("All systems finished");
    }
    size_t free_workers_count() const {
        return std::count_if(
            workers_.begin(), workers_.end(),
            [](const auto& worker) { return worker->task_count() == 0; });
    }
    const Worker* current_worker() {
        auto id = std::this_thread::get_id();
        auto it = std::find_if(workers_.begin(), workers_.end(),
                               [id](auto& w) { return w->thread_id() == id; });
        if (it != workers_.end()) return it->get();
        return nullptr;
    }
    size_t worker_count() const { return workers_.size(); }
    void submit_timeout_task(task_t&& task, time_point_t time) {
        timer_worker_->publish({std::move(task), time});
    }
    void submit_timeout_task(task_t&& task, time_duration_t delay) {
        timer_worker_->publish({std::move(task), delay});
    }
    const std::vector<const Worker*>& workers() const { return worker_crefs_; }

   private:
    void destroy_workers() {
        worker_crefs_.clear();
        timer_worker_.reset();
        std::vector<std::jthread> threads;
        for (auto& worker : workers_) {
            worker->disable_steal();
            worker->stop();
            threads.emplace_back([worker = std::move(worker)]() mutable {
                SCHEDULER_DEBUG(auto id = worker->worker_id();)
                _SCHEDULER_DEBUG("join worker {} ", id);
                worker->join();
                _SCHEDULER_DEBUG("reset worker {} ", id);
                worker.reset();
                _SCHEDULER_DEBUG("worker {} reset done", id);
            });
        }
        workers_.clear();
    }
    inline auto rand_worker() -> Worker& {
        return *workers_[std::rand() % workers_.size()];
    }
    [[gnu::no_sanitize("thread")]]
    inline std::vector<worker_paload_t> worker_paloads() const noexcept {
        std::vector<worker_paload_t> worker_paloads;
        worker_paloads.reserve(workers_.size());
        for (auto& worker : workers_) {
            worker_paloads.emplace_back(
                worker->task_count() +
                    uint32_t(worker->current_task_duration_us() /
                             StealUntilMaxUs),
                worker->max_wait_count() - worker->wait_count(),
                worker->worker_id());
        }
        return worker_paloads;
    }
    inline void work_steal(std::deque<task_t>& task_queue) {
        notify_steal();
        auto worker_paloads = this->worker_paloads();
        auto it = std::max_element(workers_.begin(), workers_.end(),
                                   [&](const auto& a, const auto& b) {
                                       return worker_paloads[a->worker_id()] <
                                              worker_paloads[b->worker_id()];
                                   });
        if (it == workers_.end()) return;
        auto& worker = **it;
        if (worker.waiting()) return;
        worker.steal(task_queue);
    }
    inline void notify_steal() {
        std::unique_lock<std::mutex> lock(steal_mutex_);
        steal_flag_ = true;
        lock.unlock();
        steal_cv_.notify_all();
    }

    void flush(std::vector<std::exception_ptr>* exceptions = nullptr) {
        systems_instencees_.erase(
            std::remove_if(systems_instencees_.begin(),
                           systems_instencees_.end(),
                           [&](std::unique_ptr<System>& sys) {
                               return sys->state_.use_count() == 1;
                           }),
            systems_instencees_.end());
    }
    void submit_expired_task(task_t&& task) {
        submit_prior_task(std::move(task));
    }

   private:
    std::vector<std::unique_ptr<System>> systems_instencees_{};
    std::vector<std::unique_ptr<Worker>> workers_{};
    std::unique_ptr<TimerWorker> timer_worker_{};
    std::vector<const Worker*> worker_crefs_{};
    std::mutex steal_mutex_{};
    std::condition_variable steal_cv_{};
    bool steal_flag_{true};
    alignas(64) std::atomic_flag sync_flag_{};
};
template <typename Derive>
inline void Promise<Derive>::submit_task(task_t&& task) {
    begin_wait();
    _SCHEDULER_DEBUG("submit {} {} remain {}", task_type(task),
                     handle_.address(), remain_task_count_.load());
    std::visit(
        [&](auto&& t) {
            using T = std::decay_t<decltype(t)>;
            if constexpr (std::is_same_v<T, FuncTask>) {
                auto bind = xc::ecs::bind_worker(task);
                scheduler()->submit_task(
                    FuncTask{[this, t = std::move(t.task_)]() {
                                 t();
                                 end_wait();
                             },
                             bind});
            } else {
                t.promise->set_parent(this);
                scheduler()->submit_task(t);
            }
        },
        task);
}
template <typename Derive>
inline void Promise<Derive>::resubmit() {
    begin_wait();
    scheduler()->submit_task(AsyncEndWaitTask{this});
}

inline void BasePromise::submit_timeout_task(task_t&& task,
                                             time_point_t until) {
    scheduler()->submit_timeout_task(std::move(task), until);
}
inline void BasePromise::submit_timeout_task(task_t&& task,
                                             time_duration_t delay) {
    scheduler()->submit_timeout_task(std::move(task), delay);
}

inline void BasePromise::async_end_wait() {
    scheduler()->submit_task(AsyncEndWaitTask{this});
}
inline void BasePromise::async_resume() {
    scheduler()->submit_task(ResumeUntilOnceTask{this});
}

}  // namespace xc::ecs