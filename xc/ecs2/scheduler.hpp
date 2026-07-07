#pragma once
#include <algorithm>
#include <cassert>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "./config.hpp"
#include "./system.hpp"
#include "./task.hpp"
#include "./types.hpp"
#include "./worker.hpp"

#pragma once
namespace xc::ecs {

class SystemScheduler {
   public:
    using system_fn_t = System (*)();
    using worker_paload_t = std::tuple<uint32_t, uint32_t, uint32_t>;
    SystemScheduler() = default;
    void add_system(System&& system) {
        auto sys = std::make_unique<System>(std::move(system));
        sys->handle.promise().scheduler_ = this;
        submit_task(ResumeUntilOnceTask{sys->handle});
        systems_instencees_.emplace_back(std::move(sys));
    }
    void start_workers(uint32_t num_workers = -1) {
        _SCHEDULER_DEBUG("Before start {} workers", num_workers);
        if (num_workers == -1) {
            num_workers = std::thread::hardware_concurrency();
        }
        _SCHEDULER_DEBUG("Start {} workers", num_workers);
        workers_.reserve(num_workers);

        for (uint32_t i = 0; i < num_workers; ++i) {
            workers_.emplace_back(std::make_unique<Worker>());
            workers_.back()->set_steal_callback(std::bind(
                &SystemScheduler::work_steal, this, std::placeholders::_1));
            workers_.back()->start(i);
        }
        _SCHEDULER_DEBUG("All workers started");
    }
    void stop_workers() {
        _SCHEDULER_DEBUG("Stop all workers");
        for (auto& worker : workers_) {
            worker->disable_steal();
            worker->stop();
        }
        for (auto& worker : workers_) {
            worker->join();
        }
        workers_.clear();
        _SCHEDULER_DEBUG("All workers stopped");
    }
    inline auto rand_worker() -> Worker& {
        return *workers_[std::rand() % workers_.size()];
    }
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
    bool try_submit_task(task_t& task) {
        _SCHEDULER_DEBUG("try submit task");
        auto bind = bind_worker(task);
        assert("bind worker is not nullptr or not in workers" &&
               (bind == nullptr ||
                std::count_if(workers_.begin(), workers_.end(),
                              [bind](auto& w) { return w.get() == bind; })) ==
                   1);
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
    void submit_task(task_t&& task) {
        while (!try_submit_task(task)) {
            std::this_thread::yield();
        }
    }
    inline void notify_steal() {
        std::unique_lock<std::mutex> lock(steal_mutex_);
        steal_flag_ = true;
        lock.unlock();
        steal_cv_.notify_all();
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
    void flush(std::vector<std::exception_ptr>* exceptions = nullptr) {
        systems_instencees_.erase(
            std::remove_if(systems_instencees_.begin(),
                           systems_instencees_.end(),
                           [&](std::unique_ptr<System>& sys) {
                               return sys->handle.done();
                           }),
            systems_instencees_.end());
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
    template <typename T>
    static bool all_has_done(const T& tasks) {
        for (auto& task : tasks) {
            if (!task.handle.done()) {
                return false;
            }
        }
        return true;
    }
    static constexpr size_t StealUntilMaxUs = 500;

    std::vector<std::unique_ptr<System>> systems_instencees_{};
    std::vector<std::unique_ptr<Worker>> workers_{};
    std::mutex steal_mutex_{};
    std::condition_variable steal_cv_{};
    bool steal_flag_{true};
    alignas(64) std::atomic_flag sync_flag_{};
};
template <typename Derive>
inline void Promise<Derive>::submit_task(task_t&& task) {
    begin_wait();
    _SCHEDULER_DEBUG("submit task {} remain {}", handle_.address(),
                     remain_task_count_.load());
    std::visit(
        [&](auto&& t) {
            using T = std::decay_t<decltype(t)>;
            if constexpr (std::is_same_v<T, FuncTask>) {
                auto bind = xc::ecs::bind_worker(task);
                scheduler_->submit_task(
                    FuncTask{[this, t = std::move(t.task_)]() {
                                 t();
                                 end_wait();
                             },
                             bind});
            } else {
                t.promise->set_parent(this);
                scheduler_->submit_task(t);
            }
        },
        task);
}
template <typename Derive>
inline void Promise<Derive>::resubmit() {
    scheduler_->submit_task(ResumeUntilOnceTask{handle_});
}
}  // namespace xc::ecs