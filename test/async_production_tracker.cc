#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "xc/async/common/production_tracker.hpp"

using namespace xc::async;

using CB = std::function<void()>;
using Tracker = ProductionTracker<CB>;
using TrackerGuard = ProducerGuard<Tracker>;
using VoidTracker = ProductionTracker<void>;
using SyncTracker = SyncProductionTracker<CB>;
using SyncVoidTracker = SyncProductionTracker<void>;

// ============================================================================
// ProductionTracker - initial state
// ============================================================================

TEST(ProductionTracker, InitialState) {
    VoidTracker tracker;
    EXPECT_TRUE(tracker.running());
    EXPECT_FALSE(tracker.closing());
    EXPECT_FALSE(tracker.closed());
}

TEST(ProductionTracker, InitialStateWithCallback) {
    Tracker tracker([] {});
    EXPECT_TRUE(tracker.running());
    EXPECT_FALSE(tracker.closing());
    EXPECT_FALSE(tracker.closed());
}

// ============================================================================
// ProducerGuard - basic lifecycle
// ============================================================================

TEST(ProductionTracker, SpawnGuardIsProducible) {
    VoidTracker tracker;
    auto guard = tracker.spawn();
    EXPECT_TRUE(guard.producible());
    EXPECT_TRUE(static_cast<bool>(guard));
}

TEST(ProductionTracker, CompleteGuard) {
    VoidTracker tracker;
    auto guard = tracker.spawn();
    EXPECT_TRUE(guard.producible());
    EXPECT_TRUE(guard.complete());
    EXPECT_FALSE(guard.producible());
    EXPECT_FALSE(static_cast<bool>(guard));
    EXPECT_TRUE(tracker.running());
}

TEST(ProductionTracker, GuardDestructorCompletes) {
    VoidTracker tracker;
    {
        auto guard = tracker.spawn();
        EXPECT_TRUE(guard.producible());
    }
    EXPECT_TRUE(tracker.running());
}

TEST(ProductionTracker, DoubleCompleteIsNoop) {
    VoidTracker tracker;
    auto guard = tracker.spawn();
    EXPECT_TRUE(guard.complete());   // first complete is effective
    EXPECT_FALSE(guard.complete());  // second is a no-op (already released)
    EXPECT_FALSE(guard.complete());  // third still no-op
    EXPECT_FALSE(guard.producible());
}

// complete() returns false for a guard that was never producible
TEST(ProductionTracker, CompleteOnEmptyGuardReturnsFalse) {
    VoidTracker tracker;
    tracker.close();
    auto guard = tracker.spawn();  // spawned after close -> immediately completed
    EXPECT_FALSE(guard.producible());
    EXPECT_FALSE(guard.complete());  // nothing to release
}

// ============================================================================
// close() - no active producers
// ============================================================================

TEST(ProductionTracker, CloseWithNoProducers) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    tracker.close();
    EXPECT_TRUE(tracker.closed());
    EXPECT_FALSE(tracker.running());
    EXPECT_FALSE(tracker.closing());
    EXPECT_EQ(close_count.load(), 1);
}

// ============================================================================
// close() - with active producers
// ============================================================================

TEST(ProductionTracker, CloseWithActiveProducer) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    auto guard = tracker.spawn();
    tracker.close();
    EXPECT_TRUE(tracker.closing());
    EXPECT_FALSE(tracker.closed());
    EXPECT_EQ(close_count.load(), 0);
    guard.complete();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

TEST(ProductionTracker, GuardDestructorAfterCloseFiresCallback) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    auto guard = std::make_unique<TrackerGuard>(tracker.spawn());
    tracker.close();
    EXPECT_EQ(close_count.load(), 0);
    guard.reset();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

// ============================================================================
// spawn() after close()
// ============================================================================

TEST(ProductionTracker, SpawnAfterCloseNotProducible) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    tracker.close();
    EXPECT_EQ(close_count.load(), 1);
    auto guard = tracker.spawn();
    EXPECT_FALSE(guard.producible());
    EXPECT_EQ(close_count.load(), 1);
}

TEST(ProductionTracker, SpawnAfterCloseDoesNotChangeState) {
    VoidTracker tracker;
    tracker.close();
    EXPECT_TRUE(tracker.closed());
    auto guard = tracker.spawn();
    EXPECT_FALSE(guard.producible());
    EXPECT_TRUE(tracker.closed());
}

// ============================================================================
// Multiple producers
// ============================================================================

TEST(ProductionTracker, MultipleProducersLastCloses) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    auto g1 = tracker.spawn();
    auto g2 = tracker.spawn();
    auto g3 = tracker.spawn();
    tracker.close();
    EXPECT_EQ(close_count.load(), 0);
    g1.complete();
    EXPECT_EQ(close_count.load(), 0);
    g2.complete();
    EXPECT_EQ(close_count.load(), 0);
    g3.complete();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

// ============================================================================
// spawn() during closing state
// ============================================================================

TEST(ProductionTracker, SpawnDuringClosingImmediatelyCompletes) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    auto g1 = tracker.spawn();
    tracker.close();
    EXPECT_TRUE(tracker.closing());
    auto g2 = tracker.spawn();
    EXPECT_FALSE(g2.producible());
    EXPECT_EQ(close_count.load(), 0);
    g1.complete();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

// ============================================================================
// set_close_callback
// ============================================================================

TEST(ProductionTracker, SetCloseCallback) {
    std::atomic<int> v{0};
    Tracker tracker([&] { v = 100; });
    tracker.set_close_callback([&] { v = 200; });
    tracker.close();
    EXPECT_EQ(v.load(), 200);
}

// ============================================================================
// start() - reopen after close
// ============================================================================

TEST(ProductionTracker, StartReopensAfterClose) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    tracker.close();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
    tracker.start();
    EXPECT_TRUE(tracker.running());
    EXPECT_FALSE(tracker.closed());
    // Close again - callback fires again
    tracker.close();
    EXPECT_EQ(close_count.load(), 2);
}

TEST(ProductionTracker, StartWhileRunningIsNoop) {
    VoidTracker tracker;
    EXPECT_TRUE(tracker.running());
    tracker.start();  // idempotent
    EXPECT_TRUE(tracker.running());
}

// ============================================================================
// ProducerGuard - movability (move ctor/assign now public)
// ============================================================================

static_assert(std::is_move_constructible_v<TrackerGuard>,
              "ProducerGuard is movable");
static_assert(std::is_move_assignable_v<TrackerGuard>,
              "ProducerGuard is move-assignable");

TEST(ProductionTracker, GuardMoveConstructor) {
    VoidTracker tracker;
    auto g1 = tracker.spawn();
    EXPECT_TRUE(g1.producible());
    auto g2 = std::move(g1);
    EXPECT_FALSE(g1.producible());
    EXPECT_TRUE(g2.producible());
    g2.complete();
}

TEST(ProductionTracker, GuardMoveAssignmentCompletesCurrent) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    auto g1 = tracker.spawn();
    auto g2 = tracker.spawn();
    tracker.close();
    EXPECT_EQ(close_count.load(), 0);
    g2 = std::move(
        g1);  // g2's old slot completes (counter 2->1), g1's slot now in g2
    EXPECT_FALSE(g1.producible());
    EXPECT_TRUE(g2.producible());
    EXPECT_EQ(close_count.load(), 0);  // still one active producer
    g2.complete();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

TEST(ProductionTracker, GuardMoveAssignmentFromEmpty) {
    VoidTracker tracker;
    auto g1 = tracker.spawn();
    auto g2 = tracker.spawn();  // g2 holds a slot
    g2.complete();
    EXPECT_FALSE(g2.producible());
    g2 = std::move(g1);  // assign from a holding guard
    EXPECT_TRUE(g2.producible());
    EXPECT_FALSE(g1.producible());
    g2.complete();
}

// ============================================================================
// Concurrent stress
// ============================================================================

TEST(ProductionTracker, ConcurrentProducersThenClose) {
    std::atomic<int> close_count{0};
    Tracker tracker([&] { close_count++; });
    constexpr int num_threads = 8;
    constexpr int ops_per_thread = 5000;

    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&] {
            while (!start.load()) {
            }
            for (int j = 0; j < ops_per_thread; ++j) {
                auto guard = tracker.spawn();
                guard.complete();
            }
        });
    }
    start.store(true);
    for (auto& t : threads) t.join();
    EXPECT_TRUE(tracker.running());
    EXPECT_EQ(close_count.load(), 0);
}

TEST(ProductionTracker, ConcurrentCloseWithProducers) {
    constexpr int runs = 50;
    for (int r = 0; r < runs; ++r) {
        std::atomic<int> close_count{0};
        Tracker tracker([&] { close_count++; });
        constexpr int num_threads = 8;
        constexpr int ops_per_thread = 2000;

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < ops_per_thread; ++j) {
                    auto guard = tracker.spawn();
                    guard.complete();
                }
            });
        }
        tracker.close();
        for (auto& t : threads) t.join();
        EXPECT_TRUE(tracker.closed()) << "run " << r;
        EXPECT_GE(close_count.load(), 1) << "run " << r;
    }
}

TEST(ProductionTracker, ConcurrentSpawnsExactlyOneClose) {
    constexpr int runs = 20;
    for (int r = 0; r < runs; ++r) {
        std::atomic<int> close_count{0};
        Tracker tracker([&] { close_count++; });
        constexpr int num_threads = 8;
        constexpr int ops_per_thread = 1000;
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < ops_per_thread; ++j) {
                    auto guard = tracker.spawn();
                    guard.complete();
                }
            });
        }
        tracker.close();
        for (auto& t : threads) t.join();
        EXPECT_TRUE(tracker.closed()) << "run " << r;
        EXPECT_EQ(close_count.load(), 1) << "run " << r;
    }
}

// ============================================================================
// ProducerGuard::complete() thread safety
// complete() uses atomic CAS internally; concurrent complete() calls on the
// same guard must decrement the counter exactly once (single winner).
// ============================================================================

TEST(ProducerGuardThreadSafety, ConcurrentCompleteSingleWinner) {
    constexpr int runs = 50;
    for (int r = 0; r < runs; ++r) {
        VoidTracker tracker;
        auto guard = tracker.spawn();
        constexpr int num_threads = 8;
        std::atomic<int> winners{0};
        std::atomic<int> losers{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                if (guard.complete())
                    winners.fetch_add(1);
                else
                    losers.fetch_add(1);
            });
        }
        for (auto& t : threads) t.join();
        EXPECT_EQ(winners.load(), 1) << "run " << r;
        EXPECT_EQ(losers.load(), num_threads - 1) << "run " << r;
        EXPECT_FALSE(guard.producible());
        EXPECT_FALSE(guard.complete());  // all subsequent are no-ops
    }
}

TEST(ProducerGuardThreadSafety, ConcurrentCompleteCounterConsistent) {
    // After N concurrent completes on one guard, spawning another guard must
    // see the counter back at 1 (previous decrement happened exactly once).
    constexpr int runs = 50;
    for (int r = 0; r < runs; ++r) {
        VoidTracker tracker;
        auto guard = tracker.spawn();
        constexpr int num_threads = 8;
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] { guard.complete(); });
        }
        for (auto& t : threads) t.join();
        // Counter should be 0 now; close() with no producers fires immediately
        std::atomic<int> close_count{0};
        // can't rebind callback; just verify close goes through instantly
        tracker.close();
        EXPECT_TRUE(tracker.closed()) << "run " << r;
    }
}

TEST(ProducerGuardThreadSafety, ConcurrentCompleteAndProducible) {
    // producible() is an atomic load; safe to race with complete(). Readers
    // spin until complete() has run, so they are guaranteed to observe the
    // post-complete false state at least once.
    VoidTracker tracker;
    auto guard = tracker.spawn();
    constexpr int num_threads = 8;
    std::atomic<int> false_count{0};
    std::atomic<bool> complete_done{false};

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads - 1; ++i) {
        threads.emplace_back([&] {
            while (!complete_done.load(std::memory_order_acquire)) {
                if (!guard.producible()) false_count.fetch_add(1);
            }
        });
    }
    threads.emplace_back([&] {
        guard.complete();
        complete_done.store(true, std::memory_order_release);
    });
    for (auto& t : threads) t.join();
    EXPECT_FALSE(guard.producible());
    EXPECT_GT(false_count.load(), 0);
}

TEST(ProducerGuardThreadSafety, ConcurrentMoveAssign) {
    // Move-assigning from a holding guard into an empty guard must correctly
    // complete the source's slot exactly once, even when a concurrent
    // complete() races on the same source guard.
    constexpr int runs = 20;
    for (int r = 0; r < runs; ++r) {
        VoidTracker tracker;
        auto g1 = tracker.spawn();
        // Build an empty guard via a closed tracker (spawn-after-close yields
        // a non-producible guard).
        VoidTracker helper;
        helper.close();
        auto g2 = helper.spawn();
        EXPECT_FALSE(g2.producible());

        std::thread t1([&] { g1.complete(); });
        std::thread t2([&] { g2 = std::move(g1); });
        t1.join();
        t2.join();
        // Exactly one decrement happened on tracker's counter; close must
        // complete immediately (no leaked active producer).
        g2.complete();
        tracker.close();
        EXPECT_TRUE(tracker.closed()) << "run " << r;
    }
}

TEST(ProducerGuardThreadSafety, SpawnCompleteConcurrentOnSameTracker) {
    // Many threads spawn+complete on the same tracker concurrently; counter
    // must always return to 0 so close() fires instantly.
    constexpr int runs = 10;
    for (int r = 0; r < runs; ++r) {
        VoidTracker tracker;
        constexpr int num_threads = 8;
        constexpr int ops = 5000;
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < ops; ++j) {
                    auto guard = tracker.spawn();
                    guard.complete();
                }
            });
        }
        for (auto& t : threads) t.join();
        tracker.close();
        EXPECT_TRUE(tracker.closed()) << "run " << r;
    }
}

// ============================================================================
// SyncProductionTracker - basic sync_close
// ============================================================================

TEST(SyncProductionTracker, InitialState) {
    SyncTracker tracker([] {});
    EXPECT_TRUE(tracker.running());
    EXPECT_FALSE(tracker.closed());
    EXPECT_FALSE(tracker.closing());
}

TEST(SyncProductionTracker, SyncCloseNoProducers) {
    std::atomic<int> close_count{0};
    SyncTracker tracker([&] { close_count++; });
    tracker.sync_close();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
    // sync_close returns only after closed - already verified by reaching here
}

TEST(SyncProductionTracker, SyncCloseVoidSpecialization) {
    SyncVoidTracker tracker;
    tracker.sync_close();
    EXPECT_TRUE(tracker.closed());
}

TEST(SyncProductionTracker, SyncCloseWithActiveProducer) {
    std::atomic<int> close_count{0};
    SyncTracker tracker([&] { close_count++; });
    auto guard = tracker.spawn();
    tracker.close();  // begins close, doesn't fire (producer active)
    EXPECT_TRUE(tracker.closing());
    EXPECT_EQ(close_count.load(), 0);
    // sync_close would block until producer drains. Use wait_until_closed in a
    // thread instead.
    std::atomic<bool> waiter_done{false};
    std::thread waiter([&] {
        tracker.wait_until_closed();
        waiter_done.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(waiter_done.load());  // still blocked
    guard.complete();  // last producer -> on_close -> promise set
    waiter.join();
    EXPECT_TRUE(waiter_done.load());
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

TEST(SyncProductionTracker, WaitUntilClosedReturnsImmediatelyIfClosed) {
    SyncTracker tracker([] {});
    tracker.sync_close();
    EXPECT_TRUE(tracker.closed());
    tracker.wait_until_closed();  // should not block
    SUCCEED();
}

TEST(SyncProductionTracker, SyncCloseBlocksUntilProducerDone) {
    std::atomic<int> close_count{0};
    SyncTracker tracker([&] { close_count++; });
    auto guard = tracker.spawn();

    std::thread closer([&] { tracker.sync_close(); });

    // Give closer time to call close() and block on wait_until_closed
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_TRUE(tracker.closing());
    EXPECT_EQ(close_count.load(), 0);

    guard.complete();  // unblocks closer

    closer.join();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
}

TEST(SyncProductionTracker, StartResets) {
    std::atomic<int> close_count{0};
    SyncTracker tracker([&] { close_count++; });
    tracker.sync_close();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 1);
    tracker.start();
    EXPECT_TRUE(tracker.running());
    // Close again, promise should be fresh
    tracker.sync_close();
    EXPECT_TRUE(tracker.closed());
    EXPECT_EQ(close_count.load(), 2);
}

TEST(SyncProductionTracker, MultipleWaiters) {
    SyncTracker tracker([] {});
    auto guard = tracker.spawn();
    tracker.close();

    constexpr int num_waiters = 4;
    std::atomic<int> woken{0};
    std::vector<std::thread> waiters;
    for (int i = 0; i < num_waiters; ++i) {
        waiters.emplace_back([&] {
            tracker.wait_until_closed();
            woken.fetch_add(1);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(woken.load(), 0);
    guard.complete();
    for (auto& t : waiters) t.join();
    EXPECT_EQ(woken.load(), num_waiters);
}
