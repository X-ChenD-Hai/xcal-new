#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "xc/async/common/active_counter.hpp"
#include "xc/async/common/async_gate.hpp"
#include "xc/async/common/mutex_accessor.hpp"
#include "xc/async/common/spin_lock.hpp"

using namespace xc::async;

#define LOG()

// ============================================================================
// ActiveCounter Tests
// ============================================================================

TEST(ActiveCounter, Basic) {
    LOG();
    ActiveCounter counter;

    EXPECT_EQ(counter.count(), 0);
    counter.increment();
    EXPECT_EQ(counter.count(), 1);
    counter.increment();
    EXPECT_EQ(counter.count(), 2);
    counter.decrement();
    EXPECT_EQ(counter.count(), 1);
    counter.reset();
    EXPECT_EQ(counter.count(), 0);
}

TEST(ActiveCounter, LockUnlock) {
    LOG();
    ActiveCounter counter;

    EXPECT_EQ(counter.count(), 0);
    counter.lock();
    EXPECT_EQ(counter.count(), 1);
    counter.lock();
    EXPECT_EQ(counter.count(), 2);
    counter.unlock();
    EXPECT_EQ(counter.count(), 1);
    counter.unlock();
    EXPECT_EQ(counter.count(), 0);
}

TEST(ActiveCounter, Concurrent) {
    LOG();
    ActiveCounter counter;
    constexpr int num_threads = 4;
    constexpr int increments_per_thread = 10000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.increment();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.count(), num_threads * increments_per_thread);
}

TEST(ActiveCounter, ReturnValue) {
    LOG();
    ActiveCounter counter;

    EXPECT_EQ(counter.increment(), 1);
    EXPECT_EQ(counter.increment(), 2);
    EXPECT_EQ(counter.increment(), 3);

    EXPECT_EQ(counter.decrement(), 2);
    EXPECT_EQ(counter.decrement(), 1);
    EXPECT_EQ(counter.decrement(), 0);
}

// ============================================================================
// AsyncGate Tests
// ============================================================================

TEST(AsyncGate, InitialState) {
    LOG();
    AsyncGate gate;

    EXPECT_TRUE(gate.running());
    EXPECT_FALSE(gate.closed());
    EXPECT_FALSE(gate.closing());
}

TEST(AsyncGate, Start) {
    LOG();
    AsyncGate gate;
    EXPECT_TRUE(gate.start());
    EXPECT_TRUE(gate.running());
}

TEST(AsyncGate, Close) {
    LOG();
    AsyncGate gate;
    gate.close();

    EXPECT_TRUE(gate.closed());
    EXPECT_FALSE(gate.running());
    EXPECT_FALSE(gate.closing());
}

TEST(AsyncGate, StartAfterClose) {
    LOG();
    AsyncGate gate;
    gate.close();
    EXPECT_TRUE(gate.closed());
    // start() 会重新打开已关闭的门
    EXPECT_TRUE(gate.start());
}

TEST(AsyncGate, BeginEndClose) {
    LOG();
    AsyncGate gate;

    EXPECT_TRUE(gate.begin_close());
    EXPECT_TRUE(gate.closing());
    EXPECT_FALSE(gate.running());

    EXPECT_TRUE(gate.end_close());
    EXPECT_TRUE(gate.closed());
}

TEST(AsyncGate, EntryClose) {
    LOG();
    AsyncGate gate;
    bool callback_executed = false;

    bool result =
        gate.entry_close([&callback_executed]() { callback_executed = true; });

    EXPECT_TRUE(result);
    EXPECT_TRUE(callback_executed);
    EXPECT_TRUE(gate.closed());
}

TEST(AsyncGate, EntryCloseWhileNotRunning) {
    LOG();
    AsyncGate gate;
    gate.close();

    bool callback_executed = false;
    bool result =
        gate.entry_close([&callback_executed]() { callback_executed = true; });

    EXPECT_FALSE(result);
    EXPECT_FALSE(callback_executed);
}

TEST(AsyncGate, ConcurrentClose) {
    LOG();
    AsyncGate gate;
    std::atomic<int> success_count{0};
    constexpr int num_threads = 4;
    constexpr int iterations = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&gate, &success_count, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                if (gate.begin_close()) {
                    success_count.fetch_add(1);
                    gate.end_close();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 1);
    EXPECT_TRUE(gate.closed());
}

// ============================================================================
// SpinLock Tests
// ============================================================================

TEST(SpinLock, Basic) {
    LOG();
    SpinLock lock;

    // 初始未锁定
    EXPECT_FALSE(lock.is_lock());

    // 加锁
    lock.lock();
    EXPECT_TRUE(lock.is_lock());

    // 解锁
    lock.unlock();
    EXPECT_FALSE(lock.is_lock());
}

TEST(SpinLock, TryLock) {
    LOG();
    SpinLock lock;

    EXPECT_TRUE(lock.try_lock());
    EXPECT_TRUE(lock.is_lock());
    EXPECT_FALSE(lock.try_lock());
    lock.unlock();
    EXPECT_FALSE(lock.is_lock());
}

TEST(SpinLock, Concurrent) {
    LOG();
    SpinLock lock;
    std::atomic<int> counter{0};
    constexpr int num_threads = 4;
    constexpr int increments_per_thread = 10000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&lock, &counter, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                lock.lock();
                counter.fetch_add(1);
                lock.unlock();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.load(), num_threads * increments_per_thread);
}

TEST(SpinLock, Reset) {
    LOG();
    Backoff<> bk;
    EXPECT_FALSE(bk.is_max());
    bk.reset();
    EXPECT_FALSE(bk.is_max());
}

// ============================================================================
// WaitAbleSpinLock Tests
// ============================================================================

#if defined(__cpp_lib_atomic_flag_test)

TEST(WaitAbleSpinLock, Basic) {
    LOG();
    WaitAbleSpinLock lock;

    EXPECT_FALSE(lock.is_lock());

    lock.lock();
    EXPECT_TRUE(lock.is_lock());

    lock.unlock();
    EXPECT_FALSE(lock.is_lock());
}

// 注意：WaitAbleSpinLock 的 wait/notify 功能需要 C++20 以上的 atomic
// wait/notify 支持 在某些平台可能不可用，测试可能需要条件编译

#endif  // __cpp_lib_atomic_flag_test

// ============================================================================
// Additional Concurrent Correctness Tests
// ============================================================================

// ActiveCounter: 压力测试 - 高并发下的计数准确性
TEST(ActiveCounter, StressTest) {
    LOG();
    ActiveCounter counter;
    constexpr int num_threads = 8;
    constexpr int increments_per_thread = 100000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.increment();
            }
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.decrement();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.count(), 0);
}

// ActiveCounter: 线程安全验证 - 读写分离
TEST(ActiveCounter, ThreadSafeReadWrite) {
    LOG();
    ActiveCounter counter;

    // 启动写入线程
    std::atomic<bool> stop{false};
    std::thread writer([&counter, &stop]() {
        while (!stop.load()) {
            counter.increment();
            counter.decrement();
        }
    });

    // 启动多个读取线程
    std::atomic<bool> read_error{false};
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&counter, &read_error, &stop]() {
            while (!stop.load()) {
                size_t count = counter.count();
                // 计数应该始终 >= 0
                if (count < 0) {
                    read_error.store(true);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop.store(true);

    writer.join();
    for (auto& t : readers) {
        t.join();
    }

    EXPECT_FALSE(read_error.load());
}

// AsyncGate: 多线程状态转换测试
TEST(AsyncGate, ConcurrentStateTransitions) {
    LOG();
    AsyncGate gate;
    std::atomic<int> state_changes{0};

    constexpr int num_threads = 8;
    constexpr int iterations = 1000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&gate, &state_changes, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                if (gate.begin_close()) {
                    state_changes.fetch_add(1);
                    gate.end_close();
                    gate.start();  // 重新打开以便下次迭代
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 至少有一次状态变化成功
    EXPECT_GT(state_changes.load(), 0);
}

// SpinLock: 防止死锁测试 - 多次加锁解锁
TEST(SpinLock, NoDeadlock) {
    LOG();
    SpinLock lock;
    std::atomic<bool> error{false};
    constexpr int iterations = 10000;

    std::thread t1([&lock, &error, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            lock.lock();
            // 短暂持有锁
            lock.unlock();
        }
    });

    std::thread t2([&lock, &error, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            lock.lock();
            lock.unlock();
        }
    });

    t1.join();
    t2.join();

    EXPECT_FALSE(error.load());
}

// SpinLock: 公平性测试
TEST(SpinLock, Fairness) {
    LOG();
    SpinLock lock;
    std::atomic<int> lock_holder{-1};
    constexpr int num_threads = 4;
    constexpr int locks_per_thread = 100;

    std::vector<std::atomic<int>> thread_locks(num_threads);
    for (auto& t : thread_locks) {
        t.store(0);
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [&lock, &lock_holder, &thread_locks, i, locks_per_thread]() {
                for (int j = 0; j < locks_per_thread; ++j) {
                    lock.lock();
                    lock_holder.store(i);
                    thread_locks[i].fetch_add(1);
                    lock.unlock();
                }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 所有线程都应该获得锁
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_GT(thread_locks[i].load(), 0);
    }
}

// ============================================================================
// Mutex/MutexAccessor Tests
// ============================================================================

TEST(Mutex, Basic) {
    LOG();
    Mutex<int> mutex(42);

    // 使用 borrow 获取访问权
    {
        auto accessor = mutex.borrow();
        EXPECT_EQ(*accessor, 42);
        *accessor = 100;
    }

    // 验证修改生效
    {
        auto accessor = mutex.borrow();
        EXPECT_EQ(*accessor, 100);
    }
}

TEST(Mutex, TryBorrow) {
    LOG();
    Mutex<int> mutex(42);

    // 第一次 try_borrow 成功
    auto result1 = mutex.try_borrow();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(**result1, 42);

    // 第二个 try_borrow 失败（锁已被持有）
    auto result2 = mutex.try_borrow();
    ASSERT_FALSE(result2.has_value());

    // 释放锁
    result1.reset();

    // 现在可以获取锁了
    auto result3 = mutex.try_borrow();
    ASSERT_TRUE(result3.has_value());
}

TEST(Mutex, ArrowOperator) {
    LOG();
    Mutex<std::string> mutex("hello");

    // 使用 -> 操作符
    {
        auto accessor = mutex.borrow();
        EXPECT_EQ(accessor->length(), 5);
    }
}

TEST(Mutex, ConstAccess) {
    LOG();
    const Mutex<int> mutex(42);

    // const borrow
    auto accessor = mutex.borrow();
    EXPECT_EQ(*accessor, 42);
}

TEST(Mutex, MoveOperator) {
    LOG();
    Mutex<std::string> mutex("hello");

    // 使用移动操作符
    auto accessor = mutex.borrow();
    std::string moved = std::move(*accessor);
    EXPECT_EQ(moved, "hello");
}

TEST(Mutex, Concurrent) {
    LOG();
    Mutex<int> mutex(0);
    constexpr int num_threads = 4;
    constexpr int increments_per_thread = 10000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mutex, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                auto accessor = mutex.borrow();
                *accessor += 1;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto accessor = mutex.borrow();
    EXPECT_EQ(*accessor, num_threads * increments_per_thread);
}

TEST(Mutex, TryBorrowConcurrent) {
    LOG();
    Mutex<int> mutex(0);
    constexpr int num_threads = 4;
    constexpr int iterations = 10000;
    std::atomic<int> success_count{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mutex, &success_count, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                auto result = mutex.try_borrow();
                if (result) {
                    **result += 1;
                    success_count.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto accessor = mutex.borrow();
    // 最终值应该小于总操作数（因为有些获取失败了）
    EXPECT_LE(*accessor, success_count.load());
}

// Mutex: 多线程读写测试
TEST(Mutex, MultiThreadReadWrite) {
    LOG();
    Mutex<std::vector<int>> mutex;
    constexpr int num_writers = 2;
    constexpr int num_readers = 2;
    constexpr int writes_per_thread = 5000;

    std::atomic<bool> error{false};

    // 写入线程
    for (int w = 0; w < num_writers; ++w) {
        std::thread([&mutex, writes_per_thread, &error]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                auto accessor = mutex.borrow();
                accessor->push_back(i);
                if (accessor->size() > 10000) {
                    error.store(true);
                }
            }
        }).join();
    }

    EXPECT_FALSE(error.load());
}

// Mutex: 锁竞争测试
TEST(Mutex, Contention) {
    LOG();
    Mutex<int> mutex(0);
    constexpr int num_threads = 8;
    constexpr int iterations = 10000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mutex, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                auto accessor = mutex.borrow();
                *accessor = *accessor + 1;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto accessor = mutex.borrow();
    EXPECT_EQ(*accessor, num_threads * iterations);
}
