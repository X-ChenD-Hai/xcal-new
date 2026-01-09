#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <print>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>
#define DEBUG(...) std::println(__VA_ARGS__)
// #define WORKER_DEBUG
#ifdef WORKER_DEBUG
#define _WORKER_DEBUG DEBUG
#else
#define _WORKER_DEBUG(...)
#endif
namespace xc::ecs {
using entity_t = uint32_t;
using entity_id_t = uint32_t;
using entity_version_t = uint8_t;

namespace detail {
template <typename T>
struct EntityPropertiesImpl;
template <>
struct EntityPropertiesImpl<uint32_t> {
    static constexpr size_t IdBitCount = 24;
    static constexpr size_t VersionBitCount = 8;
    static_assert(IdBitCount + VersionBitCount <= sizeof(entity_t) * 8,
                  "IdBitCount + VersionBitCount must be less than or equal to "
                  "sizeof(entity_t) * 8");
    static constexpr entity_id_t MaxId = (1 << IdBitCount) - 1;
    static constexpr entity_version_t MaxVersion = ~entity_version_t(0);
    static constexpr entity_t IdMask = MaxId;
    static constexpr entity_t VersionMask = ((entity_t)MaxVersion)
                                            << IdBitCount;
    inline static constexpr entity_t entity(entity_id_t id,
                                            entity_version_t version) noexcept {
        return (entity_t(id) << VersionBitCount) | version;
    }
    inline static constexpr entity_id_t id(entity_t entity) noexcept {
        return entity & IdMask;
    }
    inline static constexpr entity_version_t version(entity_t entity) noexcept {
        return (entity & VersionMask) >> IdBitCount;
    }
};
template <>
struct EntityPropertiesImpl<uint64_t> {};
template <typename T>
struct EntityProperties : EntityPropertiesImpl<T> {
    static_assert(std::is_integral_v<T>,
                  "EntityProperties only supports integral types");
};
}  // namespace detail

struct Entity {
    using properties = detail::EntityProperties<entity_t>;
    inline constexpr Entity(entity_id_t id, entity_version_t version) noexcept
        : entity_(properties::entity(id, version)) {}
    inline constexpr entity_id_t id() const noexcept {
        return properties::id(entity_);
    }
    inline constexpr entity_version_t version() const noexcept {
        return properties::version(entity_);
    }

   private:
    entity_t entity_;
};

struct Table {};
template <typename... T>
struct TableStorage {};
struct Marker {};
struct System;
struct SystemPromise;
class SystemScheduler;
template <typename T>
struct Read {};
struct Yield {};
template <typename T>
struct Write {};
template <typename T>
struct ReadWrite {};
struct ChannelMarker {};
template <typename Fn, typename... Args>
struct Channel {};

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
            auto res = fn_();
            sync_flag_->clear();
            sync_flag_->notify_one();
            return res;
        }
    }
    std::atomic_flag* sync_flag_{nullptr};
    Sync<Fn, Args...> sync_;
};

template <typename T>
struct Where {};
template <typename... T>
struct Has {};

class SystemScheduler;

using system_handle_t = std::coroutine_handle<SystemPromise>;
struct System {
    using promise_type = SystemPromise;
    System(system_handle_t handle) noexcept : handle(handle) {}
    system_handle_t handle;
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
namespace utility {
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

}  // namespace utility

template <typename T, typename Fn>
struct AsyncForeach {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    T begin;
    T end;
    Fn fn;
};

template <typename T, typename Fn>
struct AsyncForeachWait {
    using category = std::iterator_traits<T>::iterator_category;
    AsyncForeachWait(AsyncForeach<T, Fn>&& foreach, SystemScheduler* scheduler)
        : foreach_(std::move(foreach)), scheduler(scheduler) {}

    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<SystemPromise>) {}
    void await_resume();

    AsyncForeach<T, Fn> foreach_;
    SystemScheduler* scheduler{nullptr};
};

struct SystemPromise {
    System get_return_object() {
        return System{system_handle_t::from_promise(*this)};
    };
    std::suspend_always initial_suspend() { return {}; }
    SystemWait await_suspend(
        std::coroutine_handle<SystemPromise> caller) noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { exception_ = std::current_exception(); }

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
        return {std::move(foreach), scheduler};
    }

    std::suspend_always yield_value(Yield);

    void return_void() {};
    SystemScheduler* scheduler{nullptr};
    std::atomic_flag doing_{};
    std::exception_ptr exception_{nullptr};
};

struct Worker {
    Worker() = default;
    void worker() {
        auto tread_id_ = std::this_thread::get_id();
        _WORKER_DEBUG("Worker {} start @ {}", worker_id_, tread_id_);
        while (run_flag_.test()) {
            if (wait_count_ > max_wait_count_) {
                if (wait_flag_.test_and_set()) {
                    wait_count_ = 0;
                    continue;
                }
                _WORKER_DEBUG("Worker {} wait ", worker_id_);
                wait_flag_.wait(true);
                _WORKER_DEBUG("Worker {} wake up", worker_id_);
                wait_count_ = 0;
            }
            std::function<void(void)> task;
            if (task_doing_flag_.test_and_set()) {
                continue;
            }
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
                wait_count_ = 0;
            } else {
                ++wait_count_;
            }
            task_doing_flag_.clear();
            if (wait_count_ == 0) {
                _WORKER_DEBUG("worker {} tasks {}", worker_id_,
                              task_count_.load());
                task();
                --task_count_;
            }
        }
        _WORKER_DEBUG("Worker {} exit @ {}", worker_id_, tread_id_);
    }
    bool try_enqueue_task(const std::function<void(void)>& task) {
        if (task_doing_flag_.test_and_set()) {
            return false;
        }
        task_queue_.push(task);
        ++task_count_;
        task_doing_flag_.clear();
        if (wait_flag_.test_and_set()) {
            wait_flag_.clear();
            wait_flag_.notify_all();
        }
        wait_flag_.clear();
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
    auto operator<=>(const Worker& other) const {
        // 先比较是否在等待状态
        if (wait_flag_.test() != other.wait_flag_.test()) {
            return wait_flag_.test() ? std::strong_ordering::less
                                     : std::strong_ordering::greater;
        }
        return task_count_ <=> other.task_count_;
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

   private:
    std::jthread thread_{};
    uint32_t wait_count_{0};
    uint32_t max_wait_count_{1000};
    std::thread::id thread_id_{};
    std::queue<std::function<void(void)>> task_queue_{};
    std::atomic_uint32_t task_count_{0};
    uint32_t worker_id_{0};
    alignas(64) std::atomic_flag run_flag_{};
    alignas(64) std::atomic_flag wait_flag_{};
    alignas(64) std::atomic_flag task_doing_flag_{};
};
struct SystemScheduler {
    using system_fn_t = System (*)();
    SystemScheduler() = default;
    void add_system(system_fn_t system) {
        systems_.push_back(system);
        systems_instencees_.push_back(system());
        systems_instencees_.back().handle.promise().scheduler = this;
        submit_handle(systems_instencees_.back().handle);
    }
    void start_workers(uint32_t num_workers = -1) {
        if (num_workers == -1) {
            num_workers = std::thread::hardware_concurrency();
        }
        std::println("Start {} workers", num_workers);
        workers_.reserve(num_workers);

        for (uint32_t i = 0; i < num_workers; ++i) {
            workers_.emplace_back(std::make_unique<Worker>());
            workers_.back()->start(i);
        }
        std::println("async workers started");
    }
    void stop_workers() {
        std::println("Stop all workers");
        for (auto& worker : workers_) {
            worker->stop();
        }
        for (auto& worker : workers_) {
            worker->join();
        }
        std::println("All workers stopped");
    }
    void submit_task(const std::function<void(void)>& task) {
        while (1) {
            auto worker = std::min_element(
                workers_.begin(), workers_.end(),
                [](const auto& a, const auto& b) { return *a < *b; });
            if (worker != workers_.end())
                if ((*worker)->try_enqueue_task(task)) break;
        }
    }
    void submit_handle(system_handle_t handle) {
        auto task = [handle, this]() {
            while (handle.promise().doing_.test_and_set()) {
                std::this_thread::yield();
            }
            if (!handle.done()) handle.resume();
            handle.promise().doing_.clear();
        };
        submit_task(task);
    }
    void update() {
        std::println("Run scheduler");
        bool all_finish = true;
        assert(!workers_.empty());
        do {
            all_finish = true;
            for (auto system : systems_instencees_) {
                if (system.handle.promise().doing_.test_and_set()) {
                    all_finish = false;
                    break;
                }
                if (!system.handle.done()) {
                    all_finish = false;
                    system.handle.promise().doing_.clear();
                    break;
                }
                system.handle.promise().doing_.clear();
            }
            std::this_thread::yield();
        } while (!all_finish);
        systems_instencees_.clear();
        std::println("All systems finished");
    }
    size_t free_workers_count() const {
        return std::count_if(
            workers_.begin(), workers_.end(),
            [](const auto& worker) { return worker->task_count() == 0; });
    }
    template <typename T>
    static void wait_until_all(T tasks) {
        bool all_finish;
        do {
            all_finish = true;
            for (auto& task : tasks) {
                if (task.handle.done()) {
                    continue;
                }
                all_finish = false;
            }
            std::this_thread::yield();
        } while (!all_finish);
    }
    template <typename T>
    static auto wait_until_one(T tasks) {
        bool one_finish = false;
        do {
            for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                if (it->handle.done()) {
                    return it;
                }
            }
            std::this_thread::yield();
        } while (1);
        return tasks.end();
    }

    std::vector<System (*)()> systems_{};
    std::vector<System> systems_instencees_{};
    std::atomic_flag sync_flag_{};
    std::vector<std::unique_ptr<Worker>> workers_{};
};
std::suspend_always SystemPromise::yield_value(Yield) {
    scheduler->submit_handle(system_handle_t::from_promise(*this));
    return {};
}
template <typename Fn, typename... Args>
SyncWait<Fn, Args...>::SyncWait(Sync<Fn, Args...>&& sync,
                                SystemScheduler* scheduler)
    : sync_(std::move(sync)), sync_flag_(&scheduler->sync_flag_) {}

class World {};

struct Controler {};

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
    std::println("await_sync start");
    co_await Sync{[]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::println("sync call");
    }};
    std::println("await_sync end");
}

System await_foreach() {
    std::vector<int> vec{10};
    vec.resize(20, 0);
    for (int i = 0; i < vec.size(); ++i) {
        vec[i] = i;
    }
    co_await AsyncForeach{vec.begin(), vec.end(),
                          [](int i) { std::println("foreach {}", i); }};
}
template <typename T, typename Fn>
void AsyncForeachWait<T, Fn>::await_resume() {
    if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) {
        auto count = foreach_.end - foreach_.begin;
        auto worker_count = scheduler->free_workers_count();
        auto mod = count % worker_count;
        auto count_per_worker =
            (count + worker_count - 1) / worker_count - (mod != 0);
        auto tasks = std::vector<System>{};
        for (uint32_t i = 0; i < worker_count; ++i) {
            auto a = foreach_.begin + i * count_per_worker;
            auto b = (i != worker_count - 1)
                         ? foreach_.begin + (count_per_worker * (i + 1))
                         : foreach_.end;
            auto task = [](auto it, auto b, auto& fn) -> System {
                for (; it < b; ++it) fn(*it);
                co_return;
            };
            tasks.emplace_back(task(a, b, foreach_.fn));
            scheduler->submit_handle(tasks.back().handle);
        }
        scheduler->wait_until_all(tasks);
    } else {
        for (T it = foreach_.begin; it != foreach_.end; ++it) {
            foreach_.fn(*it);
        }
    }
}

int main() {
    utility::ClockRecord clock_record;
    SystemScheduler scheduler;
    clock_record.record();
    scheduler.start_workers(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.add_system(my_task);
    scheduler.add_system(await_sync);
    scheduler.add_system(await_foreach);

    for (int i = 0; i < 200; ++i) {
        scheduler.add_system(my_task);
    }
    scheduler.update();
    std::println("run using {} s", clock_record.duration());
    clock_record.record();
    scheduler.stop_workers();
    std::println("stop using {} s", clock_record.duration());
    return 0;
}
