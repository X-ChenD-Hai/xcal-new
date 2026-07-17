#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

namespace xc::ecs::structure {

template <size_t page_size = 512>
class ConsumeTokenPool;
template <size_t page_size = 512>
struct ConsumeTokenSlot;
template <size_t page_size = 512>
class Consumer;
template <size_t page_size = 512>
class ConsumeToken;
template <size_t page_size>
struct alignas(64) ConsumeTokenSlot {
    friend class ConsumeTokenPool<page_size>;
    friend class ConsumeToken<page_size>;
    friend class Consumer<page_size>;

   private:
    ConsumeTokenPool<page_size>* pool{};
    std::atomic_size_t next{};
    size_t id{};
    std::atomic_size_t ref_count;
    std::atomic_size_t expected_consume_count;
    std::atomic_size_t consuming_count;
};
template <size_t page_size>
class ConsumeToken {
    friend class Consumer<page_size>;

   public:
    using Consumer = Consumer<page_size>;
    ConsumeToken(const ConsumeToken& other) : slot_(other.slot_) {
        slot_->ref_count.fetch_add(1, std::memory_order_relaxed);
    }
    ConsumeToken& operator=(const ConsumeToken& other) {
        if (this == &other) return *this;
        release();
        slot_ = other.slot_;
        slot_->ref_count.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }
    ConsumeToken(ConsumeToken&& other) : slot_(other.slot_) {
        other.slot_ = nullptr;
    }
    ConsumeToken& operator=(ConsumeToken&&) = delete;
    ConsumeToken(ConsumeTokenSlot<page_size>* slot, size_t expected)
        : slot_(slot) {
        slot_->ref_count.fetch_add(1, std::memory_order_relaxed);
        slot_->expected_consume_count.store(expected,
                                            std::memory_order_relaxed);
    }
    ConsumeToken(ConsumeTokenPool<page_size>* pool, size_t expected);
    ConsumeToken(size_t expected) : ConsumeToken(thread_pool.get(), expected) {}
    operator bool() const { return slot_ != nullptr; }
    ConsumeToken() = default;
    ~ConsumeToken();

    size_t consume() {
        if (!slot_) return 0;
        auto expected =
            slot_->expected_consume_count.load(std::memory_order_relaxed);
        do {
            if (expected == 0) {
                return 0;
            }
        } while (slot_->expected_consume_count.compare_exchange_strong(
            expected, expected - 1, std::memory_order_acq_rel,
            std::memory_order_relaxed));
        return expected + 1;
    }
    size_t remaining() const {
        return slot_->expected_consume_count.load(std::memory_order_acquire);
    }
    void release();
    size_t inc_expected(size_t count = 1) {
        return slot_->expected_consume_count.fetch_add(
            count, std::memory_order_release);
    }
    size_t consuming_count() const {
        if (!slot_) return 0;
        return slot_->consuming_count.load(std::memory_order_acquire);
    }
    void consume_all() {
        if (!slot_) return;
        while (consume());
        while (consuming_count());
    }
    size_t ref_count() const noexcept {
        return slot_->ref_count.load(std::memory_order_acquire);
    }

    Consumer lock();

   private:
    ConsumeTokenSlot<page_size>* slot_{nullptr};
    static thread_local std::unique_ptr<ConsumeTokenPool<page_size>>
        thread_pool;
};
template <size_t page_size>
class Consumer {
    static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

   public:
    using Token = ConsumeToken<page_size>;
    Consumer(const ConsumeToken<page_size>& token, size_t id)
        : id_(id), token_(token) {
        if (id != INVALID_ID)
            token_.slot_->consuming_count.fetch_add(1,
                                                    std::memory_order_relaxed);
    }
    ~Consumer() { release(); }
    size_t id() const { return id_; }

    const Token& token() const noexcept { return token_; }

    operator bool() { return id_ != INVALID_ID; }

    void release() {
        if (id_ != INVALID_ID) {
            token_.slot_->consuming_count.fetch_sub(1,
                                                    std::memory_order_relaxed);
            token_.release();
        }
        id_ = INVALID_ID;
    }

   public:
    Consumer(const Consumer&) = delete;
    Consumer& operator=(const Consumer&) = delete;
    Consumer& operator=(Consumer&&) = delete;
    Consumer(Consumer&& other) : id_(other.id_), token_(other.token_) {
        other.id_ = INVALID_ID;
        other.token_ = ConsumeToken<page_size>();
    }
    Consumer() = default;

   private:
    size_t id_{INVALID_ID};
    ConsumeToken<page_size> token_{};
};
template <size_t page_size>
inline Consumer<page_size> ConsumeToken<page_size>::lock() {
    if (auto id = consume()) {
        return Consumer(*this, id);
    }
    return Consumer();
}
template <size_t page_size>
class ConsumeTokenPool {
    friend class ConsumeToken<page_size>;
    static constexpr size_t PageSize = page_size;
    static_assert((PageSize & PageSize - 1) == 0,
                  "PageSize must be power of 2");
    static constexpr size_t MASK = PageSize - 1;
    static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

    using Page = std::array<ConsumeTokenSlot<page_size>, PageSize>;

   public:
    ConsumeTokenPool() {}
    ConsumeToken<page_size> borrow(size_t expected = 1) {
        return ConsumeToken{borrow_slot(), expected};
    }

   private:
    void restore(const ConsumeTokenSlot<page_size>* sl) {
        if (sl == nullptr) return;
        if (sl->pool != this) {
            sl->pool->restore(sl);
            return;
        }
        auto slot = slot_at(sl->id);
        slot->ref_count.store(0, std::memory_order_relaxed);
        slot->consuming_count.store(0, std::memory_order_relaxed);
        size_t expected = next_.load(std::memory_order_relaxed);
        do {
            slot->next.store(expected, std::memory_order_release);
        } while (!next_.compare_exchange_strong(expected, slot->id,
                                                std::memory_order_acq_rel,
                                                std::memory_order_relaxed));
    }
    size_t capacity() const {
        return capacity_.load(std::memory_order_acquire);
    }
    // only safe to call in single thread using for SPMC
    ConsumeTokenSlot<page_size>* borrow_slot() {
        auto expected = next_.load(std::memory_order_relaxed);
        ConsumeTokenSlot<page_size>* slot{nullptr};
        do {
            if (expected >= capacity()) {
                new_page();
                expected = next_.load(std::memory_order_acquire);
            }
            slot = slot_at(expected);
        } while (!next_.compare_exchange_strong(
            expected, slot->next.load(std::memory_order_acquire),
            std::memory_order_acq_rel, std::memory_order_acquire));

        return slot;
    }

    inline Page& page_at(size_t id) const {
        std::lock_guard lk{page_mtx_};
        return *(pages_[id / PageSize]);
    }
    inline ConsumeTokenSlot<page_size>* slot_at(size_t id) const {
        return &page_at(id)[id & MASK];
    }

    void new_page() {
        // 先增加容量计数，确保其他线程能安全地检查容量
        auto old_capacity =
            capacity_.fetch_add(PageSize, std::memory_order_acq_rel);
        auto page = std::make_unique<Page>();
        const auto n = old_capacity;
        for (auto i = 0; i < PageSize; ++i) {
            auto& slot = (*page)[i];
            slot.pool = this;
            slot.ref_count.store(0, std::memory_order_relaxed);
            slot.expected_consume_count.store(0, std::memory_order_relaxed);
            slot.consuming_count.store(0, std::memory_order_relaxed);
            slot.next.store(n + i + 1, std::memory_order_relaxed);
            slot.id = n + i;
        }
        auto expected = next_.load(std::memory_order_relaxed);
        Page* next_page_;
        {
            std::lock_guard lock(page_mtx_);
            next_page_ = pages_.emplace_back(std::move(page)).get();
        }
        do {
            (*next_page_)[PageSize - 1].next.store(expected,
                                                   std::memory_order_release);
        } while (!next_.compare_exchange_strong(
            expected, n, std::memory_order_acq_rel, std::memory_order_relaxed));
    }

   private:
    std::vector<std::unique_ptr<Page>> pages_;
    std::atomic_size_t capacity_{0};
    std::atomic_size_t next_{INVALID_ID};
    mutable std::mutex page_mtx_{};
};

template <size_t page_size>
inline ConsumeToken<page_size>::~ConsumeToken() {
    release();
}
template <size_t page_size>
inline void ConsumeToken<page_size>::release() {
    if (slot_ == nullptr) return;
    if (slot_->ref_count.fetch_sub(1, std::memory_order_relaxed) == 1) {
        slot_->pool->restore(slot_);
    }
    slot_ = nullptr;
}
template <size_t page_size>
inline thread_local std::unique_ptr<ConsumeTokenPool<page_size>>
    ConsumeToken<page_size>::thread_pool{
        std::make_unique<ConsumeTokenPool<page_size>>()};
template <size_t page_size>
inline ConsumeToken<page_size>::ConsumeToken(ConsumeTokenPool<page_size>* pool,
                                             size_t expected)
    : ConsumeToken(pool->borrow_slot(), expected) {}

ConsumeToken() -> ConsumeToken<>;
ConsumeToken(size_t) -> ConsumeToken<>;
}  // namespace xc::ecs::structure