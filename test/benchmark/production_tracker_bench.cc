#include <benchmark/benchmark.h>

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

#include "xc/async/common/production_tracker.hpp"

using namespace xc::async;

// ============================================================================
// spawn + complete (single thread, no close)
// Measures the fast path: increment counter, then decrement.
// ============================================================================

static void BM_ProductionTracker_SpawnComplete(benchmark::State& state) {
    ProductionTracker<void> tracker;
    for (auto _ : state) {
        auto guard = tracker.spawn();
        guard.complete();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_SpawnComplete);

// Same but with a close callback (function type, not void)
static void BM_ProductionTracker_SpawnCompleteWithCallback(
    benchmark::State& state) {
    ProductionTracker<std::function<void()>> tracker([] {});
    for (auto _ : state) {
        auto guard = tracker.spawn();
        guard.complete();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_SpawnCompleteWithCallback);

// spawn + destructor (RAII complete)
static void BM_ProductionTracker_SpawnDestruct(benchmark::State& state) {
    ProductionTracker<void> tracker;
    for (auto _ : state) {
        auto guard = tracker.spawn();
        benchmark::DoNotOptimize(guard);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_SpawnDestruct);

// ============================================================================
// spawn + complete (concurrent)
// ============================================================================

static void BM_ProductionTracker_SpawnCompleteConcurrent(
    benchmark::State& state) {
    ProductionTracker<void> tracker;
    for (auto _ : state) {
        auto guard = tracker.spawn();
        guard.complete();
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_ProductionTracker_SpawnCompleteConcurrent)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK(BM_ProductionTracker_SpawnCompleteConcurrent)
    ->Threads(8)
    ->UseRealTime();

// ============================================================================
// close() with no active producers (close-only fast path)
// ============================================================================

static void BM_ProductionTracker_CloseNoProducers(benchmark::State& state) {
    for (auto _ : state) {
        ProductionTracker<void> tracker;
        tracker.close();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_CloseNoProducers);

static void BM_ProductionTracker_CloseNoProducersWithCallback(
    benchmark::State& state) {
    std::atomic<int> count{0};
    for (auto _ : state) {
        ProductionTracker<std::function<void()>> tracker(
            [&] { count.fetch_add(1, std::memory_order_relaxed); });
        tracker.close();
    }
    benchmark::DoNotOptimize(count.load());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_CloseNoProducersWithCallback);

// ============================================================================
// close() with one active producer (last-out fires callback)
// ============================================================================

static void BM_ProductionTracker_CloseWithProducer(benchmark::State& state) {
    std::atomic<int> count{0};
    for (auto _ : state) {
        ProductionTracker<std::function<void()>> tracker(
            [&] { count.fetch_add(1, std::memory_order_relaxed); });
        auto guard = tracker.spawn();
        tracker.close();
        guard.complete();
    }
    benchmark::DoNotOptimize(count.load());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProductionTracker_CloseWithProducer);

// ============================================================================
// Concurrent close racing with producers (the bug-fixed path)
// ============================================================================

static void BM_ProductionTracker_ConcurrentCloseRace(benchmark::State& state) {
    std::atomic<int> close_count{0};
    for (auto _ : state) {
        ProductionTracker<std::function<void()>> tracker(
            [&] { close_count.fetch_add(1, std::memory_order_relaxed); });
        auto guard = tracker.spawn();
        tracker.close();
        guard.complete();
    }
    state.SetItemsProcessed(state.iterations() * state.threads());
}
BENCHMARK(BM_ProductionTracker_ConcurrentCloseRace)->Threads(4)->UseRealTime();

// ============================================================================
// SyncProductionTracker - sync_close (single thread)
// Measures construct + sync_close (with promise/future overhead)
// ============================================================================

static void BM_SyncProductionTracker_SyncCloseNoProducers(
    benchmark::State& state) {
    for (auto _ : state) {
        SyncProductionTracker<void> tracker;
        tracker.sync_close();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SyncProductionTracker_SyncCloseNoProducers);

static void BM_SyncProductionTracker_SyncCloseWithCallback(
    benchmark::State& state) {
    std::atomic<int> count{0};
    for (auto _ : state) {
        SyncProductionTracker<std::function<void()>> tracker(
            [&] { count.fetch_add(1, std::memory_order_relaxed); });
        tracker.sync_close();
    }
    benchmark::DoNotOptimize(count.load());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SyncProductionTracker_SyncCloseWithCallback);

// SyncProductionTracker: spawn + complete (vs base ProductionTracker)
static void BM_SyncProductionTracker_SpawnComplete(benchmark::State& state) {
    SyncProductionTracker<void> tracker;
    for (auto _ : state) {
        auto guard = tracker.spawn();
        guard.complete();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SyncProductionTracker_SpawnComplete);

// SyncProductionTracker: spawn + sync_close + complete (blocking wait)
static void BM_SyncProductionTracker_SyncCloseWithProducer(
    benchmark::State& state) {
    std::atomic<int> count{0};
    for (auto _ : state) {
        SyncProductionTracker<std::function<void()>> tracker(
            [&] { count.fetch_add(1, std::memory_order_relaxed); });
        auto guard = tracker.spawn();
        tracker.close();
        guard.complete();
        tracker.wait_until_closed();
    }
    benchmark::DoNotOptimize(count.load());
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SyncProductionTracker_SyncCloseWithProducer);

// ============================================================================
// SyncProductionTracker - concurrent wait_until_closed
// One producer thread, N waiter threads blocked on wait_until_closed
// ============================================================================

static void BM_SyncProductionTracker_ConcurrentWaiters(
    benchmark::State& state) {
    const int num_waiters = state.range(0);
    for (auto _ : state) {
        SyncProductionTracker<void> tracker;
        auto guard = tracker.spawn();
        tracker.close();

        std::vector<std::thread> waiters;
        waiters.reserve(num_waiters);
        for (int i = 0; i < num_waiters; ++i) {
            waiters.emplace_back([&] { tracker.wait_until_closed(); });
        }
        guard.complete();
        for (auto& t : waiters) t.join();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SyncProductionTracker_ConcurrentWaiters)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->UseRealTime();

// ============================================================================
// ProducerGuard::complete() thread-safety
// N threads race to complete() the SAME guard; only one wins (decrements the
// counter), the rest observe an already-null tracker and return false.
// NOTE: per-iteration std::thread creation is included in the measurement,
// so absolute numbers reflect thread-spawn cost, not just the CAS race.
// The relative scaling across thread counts shows the contention behaviour.
// ============================================================================

static void BM_ProducerGuard_ConcurrentCompleteRace(benchmark::State& state) {
    const int num_threads = state.range(0);
    ProductionTracker<void> tracker;
    std::atomic<int> winners{0};
    for (auto _ : state) {
        auto guard = tracker.spawn();
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                if (guard.complete()) winners.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : threads) t.join();
    }
    state.counters["winners"] = winners.load();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProducerGuard_ConcurrentCompleteRace)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->UseRealTime();

BENCHMARK_MAIN();
