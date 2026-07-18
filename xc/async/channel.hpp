#pragma once
#include <atomic>
#include <deque>

#include "./promise.hpp"
#include "./structure/consume_token.hpp"
#include "./structure/ring_buffer.hpp"
#include "./system.hpp"

namespace xc::async {
template <typename T, size_t N>
class Channel {
   public:
    Channel(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel& operator=(Channel&&) = delete;

   public:
    struct RecvWait;
    struct TryRecvWait {
       public:
        TryRecvWait(const TryRecvWait&) = delete;
        TryRecvWait& operator=(const TryRecvWait&) = delete;
        TryRecvWait& operator=(TryRecvWait&&) = delete;
        TryRecvWait(TryRecvWait&& o) : channel(o.channel) {
            std::swap(v, o.v);
            std::swap(promise, o.promise);
        }

       public:
        TryRecvWait(Channel& ch) : channel(ch) {}
        constexpr bool await_ready() noexcept {
            if (channel.closed_.load(std::memory_order_acquire)) return true;
            v = channel.try_recv();
            return v.has_value();
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            promise = &handle.promise();
            channel.add_recv(this, until_, with_timeout_);
        }
        std::optional<T>&& await_resume() noexcept { return std::move(v); }
        TryRecvWait&& until(time_duration_t times) {
            with_timeout_ = true;
            until_ = time_point_t::clock::now() + times;
            return std::move(*this);
        }

        ~TryRecvWait() {}
        Channel& channel;
        std::optional<T> v{};
        BasePromise* promise{nullptr};
        time_point_t until_{};
        bool with_timeout_{false};
    };
    struct RecvWait : public TryRecvWait {
        T&& await_resume() noexcept {
            return std::move(TryRecvWait::v.value());
        }
    };
    struct TrySendWait {
       public:
        TrySendWait(const TrySendWait&) = delete;
        TrySendWait& operator=(const TrySendWait&) = delete;
        TrySendWait& operator=(TrySendWait&&) = delete;
        TrySendWait(TrySendWait&& o) : channel(o.channel) {
            std::swap(value, o.value);
            std::swap(promise, o.promise);
        }

       public:
        TrySendWait(Channel& ch, T&& value);
        constexpr bool await_ready() noexcept {
            if (channel.closed_.load(std::memory_order_acquire)) return true;
            return sended_ = channel.try_send(value);
        }
        template <IsPromise P>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            promise = &handle.promise();
            channel.add_send(this, until_, with_timeout_);
        }
        bool await_resume() noexcept { return sended_; }

        TrySendWait&& until(time_duration_t times) {
            with_timeout_ = true;
            until_ = time_point_t::clock::now() + times;
            return std::move(*this);
        }

        Channel& channel;
        BasePromise* promise{nullptr};
        T value;
        time_point_t until_{};
        bool with_timeout_{false};
        bool sended_{false};
    };
    Channel() {
        recv_flag_.clear();
        send_flag_.clear();
    }
    ~Channel() {
        close();
        consume_token.consume_all();
    }
    TryRecvWait recv() { return TryRecvWait(*this); }
    template <typename... Args>
    TrySendWait send(Args&&... args) {
        return TrySendWait(*this, std::forward<Args>(args)...);
    }
    std::optional<T> try_recv();
    bool try_send(T& value);
    void close() {
        if (closed_.load(std::memory_order_acquire)) return;
        // 1. 先尝试将消息发送给等待的接收者
        // 循环直到：消息为空 OR 接收者为空
        while (true) {
            auto msg_count = remain_msg_count_.load(std::memory_order_acquire);
            if (msg_count == 0) break;

            // 检查是否有等待的接收者
            while (recv_flag_.test_and_set());
            bool has_receiver = !recv_promises_.empty();
            recv_flag_.clear();

            if (!has_receiver) break;

            // 尝试发送消息给接收者
            notify_recv();

            // 再次检查消息数量
            if (remain_msg_count_.load(std::memory_order_acquire) == 0) break;
        }

        // 2. 唤醒所有剩余的发送等待者（让他们失败）
        while (send_flag_.test_and_set());
        while (!send_promises_.empty()) {
            auto* s = send_promises_.front();
            send_promises_.pop_front();
            if (s && s->promise) {
                s->promise->async_end_wait();
            }
        }
        send_flag_.clear();

        // 3. 唤醒所有接收等待者（让他们失败）
        while (recv_flag_.test_and_set());
        while (!recv_promises_.empty()) {
            auto* r = recv_promises_.front();
            recv_promises_.pop_front();
            if (r && r->promise) {
                r->promise->async_end_wait();
            }
        }
        recv_flag_.clear();

        // 4. 设置关闭状态，阻止新请求
        closed_.store(true, std::memory_order_release);
    }

   protected:
    void remove_send(TrySendWait* p, structure::ConsumeToken<> token) {
        auto cs = token.lock();
        if (!cs) return;
        while (send_flag_.test_and_set());
        auto it = std::find(send_promises_.begin(), send_promises_.end(), p);
        if (it != send_promises_.end()) {
            *it = nullptr;
        }
        send_flag_.clear();
        remain_msg_count_.fetch_sub(1, std::memory_order_relaxed);
        p->promise->async_end_wait();
    }
    void remove_recv(TryRecvWait* p, structure::ConsumeToken<> token) {
        auto cs = token.lock();
        if (!cs) return;
        while (recv_flag_.test_and_set());
        auto it = std::find(recv_promises_.begin(), recv_promises_.end(), p);
        if (it != recv_promises_.end()) {
            *it = nullptr;
            p->promise->async_end_wait();
        }
        recv_flag_.clear();
    }
    void add_recv(TryRecvWait* p, time_point_t until, bool with_timeout_) {
        if (!p) return;
        if (closed_.load(std::memory_order_acquire)) {
            p->promise->async_end_wait();
            return;
        }
        p->promise->begin_wait();
        while (recv_flag_.test_and_set());
        recv_promises_.push_back(p);
        recv_flag_.clear();
        if (with_timeout_) {
            consume_token.inc_expected();
            p->promise->submit_timeout_task(
                FuncTask{
                    std::bind(&Channel::remove_recv, this, p, consume_token)},
                until);
        }
        notify_sender();
    }
    void add_send(TrySendWait* p, time_point_t until, bool with_timeout_) {
        if (!p) return;
        if (closed_.load(std::memory_order_acquire)) {
            p->promise->async_end_wait();
            return;
        }
        p->promise->begin_wait();
        while (send_flag_.test_and_set());
        send_promises_.push_back(p);
        send_flag_.clear();
        if (with_timeout_) {
            consume_token.inc_expected();
            p->promise->submit_timeout_task(
                FuncTask{
                    std::bind(&Channel::remove_send, this, p, consume_token)},
                until);
        }
        remain_msg_count_.fetch_add(1, std::memory_order_release);
        notify_recv();
    }
    void notify_recv() {
        while (recv_flag_.test_and_set());
        while (!recv_promises_.empty()) {
            if (recv_promises_.front() == nullptr) {
                recv_promises_.pop_front();
                continue;
            }
            if (remain_msg_count_.load(std::memory_order_acquire) == 0) break;
            auto v = buffer_.try_dequeue();
            if (!v.has_value()) {
                notify_sender();
                continue;
            }
            remain_msg_count_.fetch_sub(1, std::memory_order_release);
            recv_promises_.front()->v = std::move(v.value());
            recv_promises_.front()->promise->async_end_wait();
            recv_promises_.pop_front();
        }
        recv_flag_.clear();
    }
    void notify_sender() {
        while (send_flag_.test_and_set());
        while (!send_promises_.empty()) {
            TrySendWait* s = send_promises_.front();
            if (s == nullptr) {
                send_promises_.pop_front();
                continue;
            }
            if (!buffer_.try_enqueue(s->value)) {
                if (buffer_.full()) break;
                continue;
            }
            s->sended_ = true;
            s->promise->async_end_wait();
            send_promises_.pop_front();
        }
        send_flag_.clear();
    }

   private:
    alignas(64) std::atomic_flag recv_flag_{};
    alignas(64) std::atomic_flag send_flag_{};
    structure::RingBuffer<T, N> buffer_{};
    std::deque<TrySendWait*> send_promises_{};
    std::deque<TryRecvWait*> recv_promises_{};
    structure::ConsumeToken<> consume_token{0};
    std::atomic_uint32_t remain_msg_count_{0};
    std::atomic_bool closed_{false};
};

template <typename T, size_t N>
inline Channel<T, N>::TrySendWait::TrySendWait(Channel& ch, T&& value)
    : channel(ch), value(std::move(value)) {}

template <typename T, size_t N>
inline std::optional<T> Channel<T, N>::try_recv() {
    if (closed_.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    auto v = buffer_.try_dequeue();
    if (v.has_value()) {
        remain_msg_count_.fetch_sub(1, std::memory_order_release);
    } else if (remain_msg_count_.load(std::memory_order_acquire) > 0) {
        notify_sender();
        auto v = buffer_.try_dequeue();
        if (v.has_value())
            remain_msg_count_.fetch_sub(1, std::memory_order_release);
        return v;
    }
    return v;
}

template <typename T, size_t N>
inline bool Channel<T, N>::try_send(T& value) {
    if (closed_.load(std::memory_order_acquire)) {
        return false;
    }
    auto success = buffer_.try_enqueue(value);
    if (success) {
        remain_msg_count_.fetch_add(1, std::memory_order_release);
        notify_recv();
    }
    return success;
}

}  // namespace xc::async
