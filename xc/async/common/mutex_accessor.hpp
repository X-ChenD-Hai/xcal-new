#include <optional>
#include <shared_mutex>
#include <type_traits>
namespace xc::async {
class SpinLock;

template <typename T>
class Effect {
   public:
    Effect(T fn) : fn_(fn) {}
    ~Effect() { fn_(); }

   private:
    T fn_;
};

// Forward declarations
template <typename T, typename Lock = ::xc::async::SpinLock>
class Mutex;
template <typename T, typename Lock = std::shared_mutex>
class SharedMutex;

// ============================================================================
// MutexAccessor - for regular mutexes
// ============================================================================
template <typename T, typename Lock, typename M>
class MutexAccessor {
    MutexAccessor(const MutexAccessor&) = delete;
    MutexAccessor& operator=(const MutexAccessor&) = delete;
    MutexAccessor& operator=(MutexAccessor&&) = delete;

    friend Mutex<T, Lock>;
    MutexAccessor(M* mutex) : mutex_(mutex) { mutex_->lock_.lock(); }
    MutexAccessor(M* mutex, int) : mutex_(mutex) {}

   public:
    MutexAccessor(MutexAccessor&& other) : mutex_(other.mutex_) {
        other.mutex_ = nullptr;
    }
    using value_type = std::conditional_t<std::is_const_v<M>, const T, T>;

    value_type* operator->() { return &mutex_->value_; }
    value_type& operator*() { return mutex_->value_; }
    const T& operator*() const { return mutex_->value_; }
    const T* operator->() const { return &mutex_->value_; }
    operator const T&() const { return mutex_->value_; }
    operator value_type&() & { return mutex_->value_; }
    operator value_type&&() && { return std::move(mutex_->value_); }
    template <typename Fn>
    auto unlock_entry(Fn&& f) {
        mutex_->lock_.unlock();
        Effect e{[this]() { mutex_->lock_.lock(); }};
        return f();
    }

    ~MutexAccessor() {
        if (mutex_) mutex_->lock_.unlock();
    };

   private:
    M* mutex_{nullptr};
};

template <typename T, typename Lock>
MutexAccessor(Mutex<T, Lock>&) -> MutexAccessor<T, Lock, Mutex<T, Lock>>;
template <typename T, typename Lock>
MutexAccessor(const Mutex<T, Lock>&)
    -> MutexAccessor<T, Lock, const Mutex<T, Lock>>;

// ============================================================================
// SharedMutexAccessor - for shared mutexes
// ============================================================================

// Read-only shared accessor
template <typename T, typename M>
class SharedReadAccessor {
    SharedReadAccessor(const SharedReadAccessor&) = delete;
    SharedReadAccessor& operator=(const SharedReadAccessor&) = delete;
    SharedReadAccessor& operator=(SharedReadAccessor&&) = delete;

    template <typename, typename>
    friend class SharedMutex;

    // Acquires shared lock
    SharedReadAccessor(M* mutex) : mutex_(mutex) {
        mutex_->lock_.lock_shared();
    }
    // Adopts an already-held shared lock (for try_borrow path)
    SharedReadAccessor(M* mutex, int) : mutex_(mutex) {}

   public:
    SharedReadAccessor(SharedReadAccessor&& other) : mutex_(other.mutex_) {
        other.mutex_ = nullptr;
    }

    const T* operator->() const { return &mutex_->value_; }
    const T& operator*() const { return mutex_->value_; }
    operator const T&() const { return mutex_->value_; }
    template <typename Fn>
    auto unlock_entry(Fn&& f) {
        mutex_->lock_.unlock_shared();
        Effect e{[this]() { mutex_->lock_.lock_shared(); }};
        return f();
    }
    ~SharedReadAccessor() {
        if (mutex_) mutex_->lock_.unlock_shared();
    };

   private:
    M* mutex_{nullptr};
};

// Write (exclusive) shared accessor
template <typename T, typename M>
class SharedWriteAccessor {
    SharedWriteAccessor(const SharedWriteAccessor&) = delete;
    SharedWriteAccessor& operator=(const SharedWriteAccessor&) = delete;
    SharedWriteAccessor& operator=(SharedWriteAccessor&&) = delete;

    template <typename, typename>
    friend class SharedMutex;

    // Acquires exclusive lock
    SharedWriteAccessor(M* mutex) : mutex_(mutex) { mutex_->lock_.lock(); }
    // Adopts an already-held exclusive lock (for try_borrow_mut path)
    SharedWriteAccessor(M* mutex, int) : mutex_(mutex) {}

   public:
    SharedWriteAccessor(SharedWriteAccessor&& other) : mutex_(other.mutex_) {
        other.mutex_ = nullptr;
    }

    T* operator->() { return &mutex_->value_; }
    T& operator*() { return mutex_->value_; }
    const T& operator*() const { return mutex_->value_; }
    const T* operator->() const { return &mutex_->value_; }
    operator const T&() const { return mutex_->value_; }
    operator T&() & { return mutex_->value_; }
    template <typename Fn>
    auto unlock_entry(Fn&& f) {
        mutex_->lock_.unlock();
        Effect e{[this]() { mutex_->lock_.lock(); }};
        return f();
    }

    ~SharedWriteAccessor() {
        if (mutex_) mutex_->lock_.unlock();
    };

   private:
    M* mutex_{nullptr};
};

// ============================================================================
// SharedMutex - base class for shared mutexes
// ============================================================================

template <typename T, typename Lock>
class SharedMutex {
   public:
    using read_accessor_t = SharedReadAccessor<T, SharedMutex<T, Lock>>;
    using const_read_accessor_t =
        SharedReadAccessor<T, const SharedMutex<T, Lock>>;
    using write_accessor_t = SharedWriteAccessor<T, SharedMutex<T, Lock>>;
    friend read_accessor_t;
    friend const_read_accessor_t;
    friend write_accessor_t;
    template <typename... Args>
    SharedMutex(Args&&... args) : value_(std::forward<Args>(args)...) {}

    // Default: shared read lock
    const_read_accessor_t borrow() const { return {this}; }
    // Exclusive write lock
    write_accessor_t borrow_mut() { return {this}; }

    // Try shared read lock
    std::optional<const_read_accessor_t> try_borrow() const {
        if (lock_.try_lock_shared()) {
            return const_read_accessor_t{this, 0};
        }
        return std::nullopt;
    }
    // Try exclusive write lock
    std::optional<write_accessor_t> try_borrow_mut() {
        if (lock_.try_lock()) {
            return write_accessor_t{this, 0};
        }
        return std::nullopt;
    }

   private:
    mutable Lock lock_{};
    T value_{};
};

// ============================================================================
// Mutex - primary template
// ============================================================================

template <typename T, typename Lock>
class Mutex {
   public:
    using accessor_t = MutexAccessor<T, Lock, Mutex<T, Lock>>;
    using const_accessor_t = MutexAccessor<T, Lock, const Mutex<T, Lock>>;
    template <typename... Args>
    Mutex(Args&&... args) : value_(std::forward<Args>(args)...) {}
    ~Mutex() = default;

    accessor_t borrow() { return {this}; }
    const_accessor_t borrow() const { return {this}; }
    std::optional<accessor_t> try_borrow() {
        if (lock_.try_lock()) {
            return accessor_t{this, 0};
        }
        return std::nullopt;
    }
    std::optional<const_accessor_t> try_borrow() const {
        if (lock_.try_lock()) {
            return const_accessor_t{this, 0};
        }
        return std::nullopt;
    }

    template <typename, typename, typename>
    friend class MutexAccessor;

   private:
    mutable Lock lock_{};
    T value_{};
};

// ============================================================================
// Mutex partial specialization for std::shared_mutex
// ============================================================================

template <typename T>
class Mutex<T, std::shared_mutex> : public SharedMutex<T, std::shared_mutex> {
   public:
    using SharedMutex<T, std::shared_mutex>::SharedMutex;
};

}  // namespace xc::async
