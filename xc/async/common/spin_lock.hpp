#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
namespace xc::async {
template <uint32_t max_backoff = 1024>
class Backoff;
}  // namespace xc::async
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#define __XC_ASYNC_CPU_RELAX \
    _mm_pause();  // MSVC/GCC/Clang 都认，或 __builtin_ia32_pause()
#elif defined(__aarch64__)
// AArch64：ISB 比 YIELD 更接近 pause 的短停顿语义
#define __XC_ASYNC_CPU_RELAX asm volatile("isb" ::: "memory");
#elif defined(__arm__)
// AArch32：优先 YIELD，编译器不支持时退到 ISB
#if defined(__GNUC__) || defined(__clang__)
#define __XC_ASYNC_CPU_RELAX __builtin_arm_yield();
#else
#define __XC_ASYNC_CPU_RELAX asm volatile("yield" ::: "memory");
#endif
#else
// 其他架构：别忙等烧 CPU，直接让出
#define __XC_ASYNC_CPU_RELAX std::this_thread::yield();
#endif
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#define __XC_ASYNC_RDTSC(v) \
    do {                    \
        uint64_t _v;        \
        _v = __rdtsc();     \
        v = _v;             \
    } while (0)
#elif defined(__aarch64__)
#define __XC_ASYNC_RDTSC(v)                            \
    do {                                               \
        uint64_t _v;                                   \
        asm volatile("mrs %0, cntvct_el0" : "=r"(_v)); \
        v = _v;                                        \
    } while (0)
#elif defined(__arm__)
#define __XC_ASYNC_RDTSC(v)                                    \
    do {                                                       \
        uint32_t _v;                                           \
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(_v)); \
        v = _v;                                                \
    } while (0)
#else
#define __XC_ASYNC_RDTSC(v)                                             \
    do {                                                                \
        v = std::chrono::steady_clock::now().time_since_epoch().count() \
    } while (0);
#endif

namespace xc::async {

template <uint32_t max_backoff>
class Backoff {
    static_assert((max_backoff & (max_backoff - 1)) == 0,
                  "max_backoff must be a power of 2");
    static inline uint32_t fast_rand() {
        thread_local uint32_t s = rdtsc() ^ 0x9e3779b9u;
        // xorshift32
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    static uint32_t rdtsc() {
        uint32_t v;
        __XC_ASYNC_RDTSC(v);
        return v;
    }

   public:
    inline uint32_t backoff() {
        unsigned cap = backoff_;
        uint32_t n = fast_rand() & (cap - 1);  // 随机抖动
        if (!n) n = 1;
        if (cap < max_backoff) {
            while (n--) __XC_ASYNC_CPU_RELAX;
            backoff_ = cap << 1;
        } else {
            std::this_thread::yield();
            backoff_ = 1;
        }
        return backoff_;
    }
    bool is_max() { return backoff_ >= max_backoff; }
    void reset() { backoff_ = 1; }

   private:
    uint32_t backoff_ = 1;
};
#if defined(__cpp_lib_atomic_flag_test)
class WaitAbleSpinLock;
#endif
class alignas(64) SpinLock {
#if defined(__cpp_lib_atomic_flag_test)
    friend WaitAbleSpinLock;
#endif
   public:
    SpinLock() { locked_.clear(std::memory_order_relaxed); }
    ~SpinLock() = default;
    void lock() {
        Backoff<> bk{};
        while (!try_lock_()) {
            bk.backoff();
        }
    }
    void unlock() { locked_.clear(std::memory_order_release); }
    bool try_lock() {
        Backoff<> bk{};
        while (!try_lock_()) {
            bk.backoff();
            if (bk.is_max()) return false;
        }
        return true;
    }
    bool is_lock()
#if defined(__cpp_lib_atomic_flag_test)
        const noexcept
#endif
    {
#if defined(__cpp_lib_atomic_flag_test)
        return locked_.test(std::memory_order_acquire);
#else
        if (locked_.test_and_set(std::memory_order_acquire)) {
            return true;
        }
        locked_.clear(std::memory_order_release);
#endif
        return false;
    }

   protected:
    bool try_lock_() {
#if defined(__cpp_lib_atomic_flag_test)
        if (!locked_.test(std::memory_order_relaxed)) {
            return !locked_.test_and_set(std::memory_order_acq_rel);
        }
        return false;
#else
        return !locked_.test_and_set(std::memory_order_acq_rel);
#endif
    }

   private:
    std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
};
#if defined(__cpp_lib_atomic_flag_test)
class WaitAbleSpinLock : public SpinLock {
   public:
    WaitAbleSpinLock() : SpinLock() {}
    ~WaitAbleSpinLock() = default;

    void wait_unlock()
#if defined(__cpp_lib_atomic_flag_test)
        const noexcept
#endif
    {
        if (!is_lock()) return;
        locked_.wait(true, std::memory_order_acquire);
    }
    void wait_lock()
#if defined(__cpp_lib_atomic_flag_test)
        const noexcept
#endif
    {
        if (is_lock()) return;
        locked_.wait(false, std::memory_order_acquire);
    }
    void notify_one() { locked_.notify_one(); }
    void notify_all() { locked_.notify_all(); }
};
#endif
}  // namespace xc::async

#undef __XC_ASYNC_CPU_RELAX
#undef __XC_ASYNC_RDTSC
