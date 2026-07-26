#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "xc/async/common/active_counter.hpp"
#include "xc/async/common/async_gate.hpp"
#include "xc/async/common/mutex_accessor.hpp"
#include "xc/async/common/spin_lock.hpp"

using namespace xc::async;

// ============================================================================
// ActiveCounter Benchmark
// ============================================================================

static void BM_ActiveCounter(benchmark::State& state) {
    ActiveCounter counter;
    for (auto _ : state) {
        counter.increment();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ActiveCounter);

// ============================================================================
// SpinLock Benchmark
// ============================================================================

static void BM_SpinLock(benchmark::State& state) {
    SpinLock lock;
    for (auto _ : state) {
        lock.lock();
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SpinLock);

static void BM_SpinLockConcurrent(benchmark::State& state) {
    SpinLock lock;
    std::atomic<int> counter{0};
    for (auto _ : state) {
        lock.lock();
        counter.fetch_add(1, std::memory_order_relaxed);
        lock.unlock();
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_SpinLockConcurrent)->Threads(4)->UseRealTime();

// ============================================================================
// AsyncGate Benchmark
// ============================================================================

static void BM_AsyncGateCreateClose(benchmark::State& state) {
    for (auto _ : state) {
        AsyncGate gate;
        gate.close();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AsyncGateCreateClose);

// ============================================================================
// Mutex Benchmark - single-threaded, different lock implementations
// ============================================================================

static void BM_MutexSpinlockSingle(benchmark::State& state) {
    Mutex<int, SpinLock> mutex(0);
    for (auto _ : state) {
        auto accessor = mutex.borrow();
        *accessor = 1;
        benchmark::DoNotOptimize(*accessor);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MutexSpinlockSingle);

static void BM_MutexStdMutexSingle(benchmark::State& state) {
    Mutex<int, std::mutex> mutex(0);
    for (auto _ : state) {
        auto accessor = mutex.borrow();
        *accessor = 1;
        benchmark::DoNotOptimize(*accessor);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MutexStdMutexSingle);

// ============================================================================
// Mutex Benchmark - concurrent, different lock implementations
// ============================================================================

static void BM_MutexSpinlockConcurrent(benchmark::State& state) {
    Mutex<int, SpinLock> mutex(0);
    for (auto _ : state) {
        auto accessor = mutex.borrow();
        *accessor += 1;
        benchmark::DoNotOptimize(*accessor);
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_MutexSpinlockConcurrent)->Threads(4)->UseRealTime();

static void BM_MutexStdMutexConcurrent(benchmark::State& state) {
    Mutex<int, std::mutex> mutex(0);
    for (auto _ : state) {
        auto accessor = mutex.borrow();
        *accessor += 1;
        benchmark::DoNotOptimize(*accessor);
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_MutexStdMutexConcurrent)->Threads(4)->UseRealTime();

// ============================================================================
// Mutex<std::shared_mutex> Benchmark - concurrent readers/writers
// Threads [0, num_readers) are readers; the rest are writers.
// ============================================================================

static void BM_MutexSharedMutexConcurrent(benchmark::State& state) {
    Mutex<int, std::shared_mutex> mutex(0);
    const int num_readers = 4;
    const bool is_reader = state.thread_index() < num_readers;
    for (auto _ : state) {
        if (is_reader) {
            auto accessor = mutex.borrow();
            int v = *accessor;
            benchmark::DoNotOptimize(v);
        } else {
            auto accessor = mutex.borrow_mut();
            *accessor += 1;
            benchmark::DoNotOptimize(*accessor);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_MutexSharedMutexConcurrent)->Threads(6)->UseRealTime();

BENCHMARK_MAIN();
