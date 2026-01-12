#include <float.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <complex>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <print>
#include <queue>
#include <random>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include "utility.hpp"
#include "worker.hpp"
#include "world.hpp"

namespace xc::ecs {

template <typename Fn, typename... Args>
struct Sync {
    using invoke_result_t = std::invoke_result_t<Fn, Args...>;
    Sync(Fn&& fn, Args&&... args)
        : fn_(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)) {}

    std::function<invoke_result_t(void)> fn_;
};
template <typename Fn, typename... Args>
struct SyncWait {
    using invoke_result_t = std::invoke_result_t<Fn, Args...>;
    SyncWait(Sync<Fn, Args...>&& sync, SystemScheduler* scheduler);
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<SystemPromise> handle) {}
    invoke_result_t await_resume() {
        std::println("await_resume {}", (void*)this);
        uint32_t wait_count = 0;
        while (sync_flag_->test_and_set()) {
            if (wait_count++ > 1000) {
                sync_flag_->wait(true);
            }
        }
        if constexpr (std::is_void_v<invoke_result_t>) {
            sync_.fn_();
            sync_flag_->clear();
            sync_flag_->notify_one();
        } else {
            auto res = sync_.fn_();
            sync_flag_->clear();
            sync_flag_->notify_one();
            return res;
        }
    }
    std::atomic_flag* sync_flag_{nullptr};
    Sync<Fn, Args...> sync_;
};
using system_handle_t = std::coroutine_handle<SystemPromise>;
struct System {
    using promise_type = SystemPromise;
    System(System&& o) : handle(nullptr) {
        std::println("move construct system {} @ {}", (void*)this,
                     (void*)o.handle.address());
        std::swap(handle, o.handle);
    }
    System& operator=(System&& o) {
        std::println("move assign system {} @ {}", (void*)this,
                     (void*)o.handle.address());
        std::swap(handle, o.handle);
        return *this;
    }
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System() noexcept : handle(nullptr) {
        std::println("default construct system {}", (void*)this);
    };
    System(system_handle_t handle) noexcept : handle(handle) {
        std::println("construct system {} @ {}", (void*)this,
                     (void*)handle.address());
    }
    ~System();
    system_handle_t handle{nullptr};
};
struct SystemWait {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<SystemPromise>) {}
    void await_resume() {}
};
struct SystemFinalWait {
    SystemFinalWait() noexcept {}
    ~SystemFinalWait() noexcept {}
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<SystemPromise>) noexcept {}
    void await_resume() noexcept {}
};

template <typename T>
struct ReadWait {
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<SystemPromise>) noexcept {}
    T await_resume() noexcept { return T{}; }
};

template <typename T, typename Fn>
struct AsyncForeach {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    T begin;
    T end;
    Fn fn;
};
alignas(64) std::atomic_uint32_t membad_count = 0;
template <typename T, typename Fn>
struct AsyncForeachWait final {
    AsyncForeachWait(AsyncForeach<T, Fn>&& foreach, SystemScheduler* scheduler,
                     system_handle_t handle);

    bool await_ready();
    void await_suspend(std::coroutine_handle<SystemPromise>);
    void await_resume();

    AsyncForeach<T, Fn> foreach_{};
    system_handle_t handle_{nullptr};
    system_handle_t caller_{nullptr};
    std::atomic_uint32_t working_task_count_{0};

    static void* operator new(size_t size) {
        auto max_try_count = 1000;
        while (max_try_count--) try {
                return ::operator new(size);
            } catch (const std::bad_alloc& e) {
                if (max_try_count == 0) {
                    ++membad_count;
                    std::println("AsyncForeachWait::operator new failed");
                    throw std::bad_alloc();
                }
            }
    }
    static void operator delete(void* ptr) {
        auto max_try_count = 1000;
        while (max_try_count--) try {
                ::operator delete(ptr);
            } catch (const std::bad_alloc& e) {
                if (max_try_count == 0) {
                    ++membad_count;
                    std::println("AsyncForeachWait::operator delete failed");
                    throw std::bad_alloc();
                }
            }
    }
};

template <typename T>
struct ResumeValue {
    bool await_ready() { return true; }

    void await_suspend(auto) {}

    T&& await_resume() { return std::move(value); }

    T value{};
};

struct SystemPromise {
    System get_return_object() {
        return System{handle = system_handle_t::from_promise(*this)};
    };
    std::suspend_always initial_suspend() { return {}; }
    SystemWait await_suspend(
        std::coroutine_handle<SystemPromise> caller) noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept {
        // waiting.test_and_set();
        return {};
    }

    void unhandled_exception() {
        exception_ = std::current_exception();
        // waiting.test_and_set();
    }

    template <typename T>
    ReadWait<T> yield_value(Read<T>) {
        return ReadWait<T>{};
    };
    template <typename Fn, typename... Args>
    SyncWait<Fn, Args...> await_transform(Sync<Fn, Args...>&& sync) {
        return {std::move(sync), scheduler};
    }
    template <typename T, typename Fn>
    AsyncForeachWait<T, Fn> await_transform(AsyncForeach<T, Fn>&& foreach) {
        return {std::move(foreach), scheduler, handle};
    }
    std::suspend_always yield_value(Yield);

    void return_void() {

    };

    ResumeValue<system_handle_t> yield_value(uint32_t id_) {
        id = id_;
        return {.value = system_handle_t::from_address(this)};
    }

    SystemScheduler* scheduler{nullptr};
    std::exception_ptr exception_{nullptr};
    uint32_t id{0};
    system_handle_t handle;
};

struct SystemScheduler {
    using system_fn_t = System (*)();
    using worker_paload_t = std::tuple<uint32_t, uint32_t, uint32_t>;
    SystemScheduler() = default;
    void add_system(system_fn_t system) {
        systems_.push_back(system);
        auto sys = std::make_unique<System>(system());
        sys->handle.promise().scheduler = this;
        submit_handle(sys->handle);
        systems_instencees_.emplace_back(std::move(sys));
    }
    void start_workers(uint32_t num_workers = -1) {
        _SCHEDULER_DEBUG("Before start {} workers", num_workers);
        if (num_workers == -1) {
            num_workers = std::thread::hardware_concurrency();
        }
        std::println("Start {} workers", num_workers);
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

    bool try_submit_task(const Worker::task_t& task) {
        for (auto& worker : workers_) {
            if (!worker->waiting()) continue;
            if (worker->try_enqueue_task(task)) {
                _WORKER_DEBUG("submit task to worker {} which is waiting",
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
            _WORKER_DEBUG("submit task to worker {} which is spin waiting",
                          (*min_worker)->worker_id());
            return true;
        }
        if (rand_worker().try_enqueue_task(task)) {
            _WORKER_DEBUG("submit task to worker {} which is doing other tasks",
                          rand_worker().worker_id());
            return true;
        }
        return false;
    }
    void submit_task(const Worker::task_t& task) {
        while (!try_submit_task(task)) {
            std::this_thread::yield();
        }
    }
    void submit_handle(system_handle_t handle) {
        std::println("start submit {}", handle.address());
        if (!handle) return;
        auto task = [handle, this]() {
            assert(!handle.done() && "handle is invalid");
            std::println("start resume {} from submited", handle.address());
            try {
                handle.resume();
                std::println("resume {} success with id {}", handle.address(),
                             handle.promise().id);
            } catch (...) {
                handle.promise().exception_ = std::current_exception();
                std::println("catch exception in {}", handle.address());
            }
            std::println("end resume {} from submited", handle.address());
        };
        submit_task(task);
        // handle.promise().waiting.clear();
        std::println("submit {} success", handle.address());
    }
    inline void notify_steal() {
        std::unique_lock<std::mutex> lock(steal_mutex_);
        steal_flag_ = true;
        lock.unlock();
        steal_cv_.notify_all();
    }
    inline void work_steal(std::queue<Worker::task_t>& task_queue) {
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
        auto count = worker.task_count() / 2;
        for (uint32_t i = 0; i < count; ++i) {
            Worker::task_t task;
            if (worker.try_dequeue_task(task)) {
                task_queue.push(task);
                _WORKER_DEBUG("steal task from worker {}", worker.worker_id());
            }
        }
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
        std::println("Run scheduler");
        assert(!workers_.empty());
        if (systems_instencees_.empty()) return;
        std::println("Enter Loop");
        std::vector<std::exception_ptr> exceptions;
        uint32_t redo_count_ = 0;
        do {
            {
                std::unique_lock<std::mutex> lock(steal_mutex_);
                _SCHEDULER_DEBUG("Wait for steal");
                steal_cv_.wait_for(lock, std::chrono::microseconds(100),
                                   [this]() { return steal_flag_; });
                steal_flag_ = false;
                _SCHEDULER_DEBUG("Steal done");
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
                std::println("Exception: {}", e.what());
            }
        }
        std::println("All systems finished");
    }
    size_t free_workers_count() const {
        return std::count_if(
            workers_.begin(), workers_.end(),
            [](const auto& worker) { return worker->task_count() == 0; });
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

    std::vector<System (*)()> systems_{};
    std::vector<std::unique_ptr<System>> systems_instencees_{};
    std::vector<std::unique_ptr<Worker>> workers_{};
    std::mutex steal_mutex_{};
    std::condition_variable steal_cv_{};
    bool steal_flag_{true};
    alignas(64) std::atomic_flag sync_flag_{};
};
std::suspend_always SystemPromise::yield_value(Yield) {
    scheduler->submit_handle(handle);
    return {};
}
template <typename Fn, typename... Args>
SyncWait<Fn, Args...>::SyncWait(Sync<Fn, Args...>&& sync,
                                SystemScheduler* scheduler)
    : sync_(std::move(sync)), sync_flag_(&scheduler->sync_flag_) {}

class World {};

struct Controler {};
System::~System() {
    std::println("destroy system {} @ {} {}", (void*)this,
                 (void*)handle.address(), handle ? handle.promise().id : NAN);
    assert((!handle || handle.done() || handle.promise().exception_) &&
           "system is not done");
    if (handle) handle.destroy();
}

template <typename Fn, typename T, typename Offset>
void smart_invoke(Fn&& fn, T&& it, Offset&& offset) {
    if constexpr (std::is_invocable_v<Fn, T, Offset>) {
        fn(it, offset);
    } else if constexpr (std::is_invocable_v<Fn, T>) {
        fn(it);
    } else {
        using Vt = decltype(*it);
        if constexpr (std::is_invocable_v<Fn, Vt>) {
            fn(*it);
        } else if constexpr (std::is_invocable_v<Fn, Vt, Offset>) {
            fn(*it, offset);
        } else {
            static_assert(false, "invalid call");
        }
    }
}

template <typename T, typename Fn>
AsyncForeachWait<T, Fn>::AsyncForeachWait(AsyncForeach<T, Fn>&& foreach,
                                          SystemScheduler* scheduler,
                                          system_handle_t handle)
    : foreach_(std::move(foreach)), handle_(handle) {
    std::println("AsyncForeachWait {} constructor @ {}", (void*)this,
                 (void*)handle.address());

    auto count = foreach_.end - foreach_.begin;
    auto worker_count = scheduler->worker_count();
    auto mod = count % worker_count;
    auto count_per_worker =
        (count + worker_count - 1) / worker_count - (mod != 0);
    working_task_count_.store(worker_count + 1);

    for (uint32_t i = 0; i < worker_count; ++i) {
        auto a = foreach_.begin + i * count_per_worker;
        auto b = (i == worker_count - 1) ? foreach_.end : a + count_per_worker;
        auto task = [it = a, end = b, this]() mutable {
            for (; it < end; ++it) {
                smart_invoke(foreach_.fn, it, it - foreach_.begin);
            }
            if (working_task_count_.fetch_sub(1) == 1) {
                if (caller_) {
                    _SCHEDULER_DEBUG("AsyncForeachWait await_resume");
                    caller_.resume();
                }
            }
        };
        scheduler->submit_task(task);
    }
}
template <typename T, typename Fn>
bool AsyncForeachWait<T, Fn>::await_ready() {
    auto ready = working_task_count_.load() == 1;
    if (ready)
        std::println("AsyncForeachWait {} await_ready @ {}", (void*)this,
                     handle_.address());
    else
        std::println("AsyncForeachWait {} await_ready not ready @ {}",
                     (void*)this, handle_.address());
    return ready;
}
template <typename T, typename Fn>
void AsyncForeachWait<T, Fn>::await_suspend(
    std::coroutine_handle<SystemPromise> caller) {
    std::println("AsyncForeachWait {} await_suspend @ {}", (void*)this,
                 (void*)caller.address());
    caller_ = caller;
    if (working_task_count_.fetch_sub(1) == 1) {
        caller.resume();
    }
}
template <typename T, typename Fn>
void AsyncForeachWait<T, Fn>::await_resume() {
    std::println("AsyncForeachWait {} await_resume @ {}", (void*)this,
                 handle_.address());
}
}  // namespace xc::ecs

using namespace xc::ecs;
System my_task() {
    utility::ClockRecord clock_record;
    clock_record.record();
    std::println("my_task");
    std::println("my_task yield");
    co_yield Yield{};
    std::println("my_task yield end");
    std::println("my_task using {} ms", clock_record.duration_ms());
    co_return;
}

System await_sync() {
    co_await Sync{[]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::println("sync call");
    }};
}

// alignas(64) std::atomic_uint32_t task_count = 20000;
alignas(64) std::atomic_uint32_t task_start_count = 0;
alignas(64) std::atomic_uint32_t task_finish_count = 0;

// System async_foreach() {
//     auto id = ++task_start_count;
//     auto handle = co_yield id;
//     std::println("async_foreach first resume @ {} {}", handle.address(), id);
//     std::vector<int> vec{};
//     vec.resize(144004, 0);
//     for (int i = 0; i < vec.size(); ++i) {
//         vec[i] = i;
//     }
//     utility::ClockRecord clock_record;
//     clock_record.record();
//     using namespace std::chrono_literals;
//     // std::this_thread::sleep_for((std::rand() % 10) * 10ns);
//     std::println("async_foreach start async foreach {}", id);
//     co_await AsyncForeach{vec.begin(), vec.end(), [](int& i) { i = 0; }};
//     co_await AsyncForeach{vec.begin(), vec.end(),
//                           [](int& i) { assert(i == 0); }};

//     std::println(
//         "async_foreach second reume with await_foreach using @ {2} {1} ms
//         {0}", id, clock_record.duration_ms(), handle.address());
//     --task_count;
//     ++task_finish_count;
//     std::println("call end @ {} {}", handle.address(), id);
// }

alignas(64) std::atomic_uint32_t task_count =
    100;  // 减少任务数，增加每个任务负载
alignas(64) std::atomic_uint32_t heavy_calc_counter = 0;

System heavy_workload() {
    auto id = ++task_start_count;
    auto handle = co_yield id;

    // 创建更大的数据集合
    const size_t data_size = 10'000'000;  // 1000万元素，约40MB
    std::vector<double> data(data_size);

    // 初始化数据
    utility::ClockRecord clock_record;
    clock_record.record();

    // 复杂计算负载1：矩阵变换模拟
    co_await AsyncForeach{
        data.begin(), data.end(), [](double& value) {
            // 复杂计算：模拟正弦波+噪声
            static std::mt19937 rng(std::random_device{}());
            static std::uniform_real_distribution<double> dist(-1.0, 1.0);

            double x = value;
            for (int i = 0; i < 10; ++i) {  // 多层计算增加负载
                x = std::sin(x) + dist(rng) * 0.1;
                x = std::cos(x * 0.5) + dist(rng) * 0.05;
            }
            value = x;
            ++heavy_calc_counter;
        }};

    // 复杂计算负载2：归约计算
    std::atomic<double> sum = 0.0;
    co_await AsyncForeach{
        data.begin(), data.end(), [&sum](const double& value) {
            double local_sum = 0.0;
            // 模拟复杂聚合计算
            for (int i = 0; i < 5; ++i) {
                local_sum += std::log(std::abs(value) + 1.0) *
                             std::sqrt(std::abs(value) + 1.0);
            }
            sum.fetch_add(local_sum, std::memory_order_relaxed);
            ++heavy_calc_counter;
        }};

    // 复杂计算负载3：排序模拟（部分排序）
    std::sort(data.begin(), data.begin() + data_size / 10);  // 排序前10%

    std::println(
        "Heavy task {} completed. Sum: {:.6f}, Time: {}ms, "
        "Calculations: {}",
        id, sum.load(), clock_record.duration_ms(), heavy_calc_counter.load());

    --task_count;
    ++task_finish_count;
    co_return;
}

System complex_pipeline() {
    auto id = ++task_start_count;

    // 阶段1：数据生成
    std::vector<std::complex<double>> complex_data(5'000'000);
    co_await AsyncForeach{
        complex_data.begin(), complex_data.end(), [](std::complex<double>& c) {
            static std::mt19937 rng(std::random_device{}());
            static std::uniform_real_distribution<double> dist(-10.0, 10.0);
            c = std::complex<double>(dist(rng), dist(rng));
        }};

    // 阶段2：FFT模拟计算（简化）
    utility::ClockRecord phase2;
    phase2.record();

    co_await AsyncForeach{complex_data.begin(), complex_data.end(),
                          [](std::complex<double>& c) {
                              // 模拟复数运算负载
                              for (int i = 0; i < 20; ++i) {
                                  c = std::pow(c, 1.01);  // 轻微幂运算
                                  c = std::conj(c) * std::polar(1.0, 0.01);
                              }
                          }};

    // 阶段3：结果验证
    std::atomic<double> magnitude_sum = 0.0;
    co_await AsyncForeach{complex_data.begin(), complex_data.end(),
                          [&magnitude_sum](const std::complex<double>& c) {
                              double mag = std::norm(c);
                              // 多阶段计算增加负载
                              for (int i = 0; i < 8; ++i) {
                                  mag = std::sqrt(mag) + std::log(mag + 1.0);
                              }
                              magnitude_sum.fetch_add(
                                  mag, std::memory_order_relaxed);
                          }};

    std::println("Pipeline task {}: Phase2: {}ms, Magnitude: {:.6f}", id,
                 phase2.duration_ms(), magnitude_sum.load());

    --task_count;
    ++task_finish_count;
    co_return;
}

System memory_intensive_work() {
    auto id = ++task_start_count;

    // 分配大内存并频繁访问
    const size_t matrix_size = 4096;  // 4K x 4K矩阵
    std::vector<std::vector<double>> matrix(matrix_size,
                                            std::vector<double>(matrix_size));

    utility::ClockRecord clock;
    clock.record();

    // 矩阵运算：模拟科学计算负载
    for (int iter = 0; iter < 3; ++iter) {
        // 并行初始化
        co_await AsyncForeach{matrix.begin(), matrix.end(),
                              [](std::vector<double>& row) {
                                  for (auto& val : row) {
                                      val = std::rand() / double(RAND_MAX);
                                  }
                              }};

        // 并行计算：矩阵变换
        co_await AsyncForeach{
            matrix.begin(), matrix.end(),
            [&matrix](std::vector<double>& row, size_t row_idx) {
                for (size_t col_idx = 0; col_idx < matrix.size(); ++col_idx) {
                    // 模拟邻域计算（类似卷积）
                    double sum = 0.0;
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            int ni = row_idx + di;
                            int nj = col_idx + dj;
                            if (ni >= 0 && ni < matrix.size() && nj >= 0 &&
                                nj < matrix.size()) {
                                sum += matrix[ni][nj];
                            }
                        }
                    }
                    row[col_idx] = sum / 9.0;
                }
            }};
    }

    std::println("Matrix task {} completed in {}ms", id, clock.duration_ms());

    --task_count;
    ++task_finish_count;
    co_return;
}

// 混合负载测试
System mixed_workload_test() {
    auto id = ++task_start_count;

    // 随机选择不同的工作负载模式
    int pattern = id % 3;

    switch (pattern) {
        case 0: {
            // CPU密集型：素数计算
            const int prime_limit = 1'000'000;
            std::atomic<int> prime_count = 0;

            co_await AsyncForeach{2, prime_limit, [&prime_count](int n) {
                                      bool is_prime = true;
                                      for (int i = 2; i * i <= n; ++i) {
                                          if (n % i == 0) {
                                              is_prime = false;
                                              break;
                                          }
                                      }
                                      if (is_prime) prime_count.fetch_add(1);
                                  }};
            std::println("Prime task {}: found {} primes", id,
                         prime_count.load());
            break;
        }
        case 1: {
            // 浮点密集型：数值积分
            const int steps = 10'000'000;
            std::atomic<double> integral = 0.0;

            co_await AsyncForeach{0, steps, [&integral, steps](int i) {
                                      double x = i / double(steps);
                                      integral.fetch_add(
                                          std::sin(x) * std::exp(-x),
                                          std::memory_order_relaxed);
                                  }};
            integral = integral.load() / steps;
            std::println("Integral task {}: result = {:.10f}", id,
                         integral.load());
            break;
        }
        case 2: {
            // 内存访问密集型：随机访问模式
            const size_t size = 10'000'000;
            std::vector<int> data(size);

            // 初始化
            for (auto& v : data) v = std::rand();

            // 随机访问模式
            co_await AsyncForeach{
                0ull, size, [&data](size_t i) {
                    // 随机跳跃访问（破坏缓存局部性）
                    size_t idx = (i * 9973) % data.size();  // 质数模运算
                    data[idx] = data[idx] * 1103515245 + 12345;
                }};
            std::println("Random access task {} completed", id);
            break;
        }
    }

    --task_count;
    ++task_finish_count;
    co_return;
}

int main(int argc, char* argv[]) {
    // 使用更少的任务但更大的负载
    task_count = 50;  // 50个重负载任务

    SystemScheduler scheduler{};
    scheduler.start_workers(std::thread::hardware_concurrency());

    // 混合不同类型的任务
    for (int i = 0; i < task_count; ++i) {
        if (i % 4 == 0)
            scheduler.add_system(heavy_workload);
        else if (i % 4 == 1)
            scheduler.add_system(complex_pipeline);
        else if (i % 4 == 2)
            scheduler.add_system(memory_intensive_work);
        else
            scheduler.add_system(mixed_workload_test);
    }

    scheduler.update();
    scheduler.stop_workers();

    std::println("Total calculations: {}", heavy_calc_counter.load());
    return 0;
}

// int main(int argc, char* argv[]) {
//     utility::ClockRecord app_record;
//     app_record.record();
//     auto run_count = 1;
//     auto thread_count = 10;
//     if (argc > 1) {
//         run_count = std::stoi(argv[1]);
//     }
//     if (argc > 3) {
//         thread_count = std::stoi(argv[2]);
//     }
//     for (size_t i = 0; i < run_count; ++i) {
//         {
//             utility::ClockRecord app_record;
//             app_record.record();
//             utility::ClockRecord clock_record;
//             SystemScheduler scheduler{};
//             clock_record.record();
//             scheduler.start_workers(thread_count);
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));

//             std::println("start workers use {} ms",
//             clock_record.duration_ms()); clock_record.record(); auto c =
//             task_count.load(); for (int i = 0; i < c; ++i) {
//                 scheduler.add_system(async_foreach);
//                 if (i % 100 == 1) {
//                     scheduler.flush();
//                 }
//             }
//             scheduler.update();
//             auto t = clock_record.duration_ms();
//             std::println("run using {} ms {} ms per task", t, (float)t / c);
//             clock_record.record();
//             scheduler.stop_workers();
//             std::println("task count {}", task_count.load());
//             std::println("start count {}", task_start_count.load());
//             std::println("finish count {}", task_finish_count.load());
//             std::println("membad count {}", membad_count.load());
//             std::println("stop using {} s", clock_record.duration());
//             std::println("app using {} s", app_record.duration());
//         }
//     }
//     std::println("app using {} ms", app_record.duration_ms());
//     std::println("time per run {} ms", app_record.duration_ms() / run_count);
//     assert(task_count.load() == 0);
//     // assert(task_finish_count.load() == task_count.load());
//     return 0;
// }
