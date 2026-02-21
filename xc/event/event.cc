#include "./event.hpp"

#include <cassert>
#include <iostream>
#include <mutex>
#include <print>
#include <queue>
#include <unordered_set>

#include <xc/common/xc_assert.hpp>

thread_local std::vector<EventLoop*> EventLoop::thread_loops_;
thread_local std::shared_mutex EventLoop::thread_loops_mutex_;
std::vector<EventLoop*> EventLoop::global_loops_;
std::shared_mutex EventLoop::global_loops_mutex_;

EventListener::EventListener() { EventLoop::current()->subscribe(this); };
EventListener::~EventListener() { EventLoop::current()->unsubscribe(this); };
void EventLoop::subscribe(EventListener* listener) {
    std::println("subscribe listener {} {}", (void*)this, (void*)listener);
    std::lock_guard lock(listeners_mutex_);
    listeners_.insert(listener);
}
void EventLoop::unsubscribe(EventListener* listener) {
    std::lock_guard lock(removed_listeners_mutex_);
    removed_listeners_.insert(listener);
}
bool EventLoop::flush() noexcept {
    std::unique_lock lock(sub_events_mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return false;
    }
    std::unordered_set<EventListener*> listeners;
    {
        std::lock_guard lock1(listeners_mutex_);
        listeners.swap(listeners_);
        {
            std::lock_guard lock(removed_listeners_mutex_);
            for (auto& listener : removed_listeners_) {
                listeners.erase(listener);
            }
            removed_listeners_.clear();
        }
    }
    std::queue<std::unique_ptr<AbsEvent>> events;
    {
        std::lock_guard lock(events_mutex_);
        events.swap(events_);
    }
    while (!events.empty()) {
        auto e = std::move(events.front());
        events.pop();
        if (!e) continue;
        for (auto& listener : listeners) {
            try {
                if (listener->event(e.get())) {
                    break;
                }
            } catch (std::runtime_error& e) {
                std::cerr << "Runtime error caught in event listener: "
                          << e.what() << std::endl;
            }
        }
    }
    {
        std::lock_guard lock(listeners_mutex_);
        if (!listeners_.empty())
            for (auto& listener : listeners_) {
                listeners.insert(listener);
            }
        listeners_.swap(listeners);
    }
    return true;
}
void EventLoop::publish(std::unique_ptr<AbsEvent> event) {
    std::lock_guard lock(events_mutex_);
    events_.push(std::move(event));
}
const std::unordered_set<EventListener*>& EventLoop::listeners() {
    std::lock_guard lock(listeners_mutex_);
    return listeners_;
}
const std::queue<std::unique_ptr<AbsEvent>>& EventLoop::events() {
    std::lock_guard lock(events_mutex_);
    return events_;
}
void EventLoop::clear() {
    std::lock_guard lock(events_mutex_);
    auto dsiposed_events = std::move(events_);
}
void EventPublisher::publish(std::unique_ptr<AbsEvent> event) {
    loop_->publish(std::move(event));
};
EventPublisher::EventPublisher()
    : loop_{EventLoop::current() ? EventLoop::current()
                                 : EventLoop::global_loop()} {};
EventLoop::~EventLoop() {
    while (!flush());
    std::lock_guard lock(thread_info_mutex_);
    for (auto it : thread_loop_info_) {
        std::lock_guard lock(*it.second);
        it.first->erase(std::remove(it.first->begin(), it.first->end(), this),
                        it.first->end());
    }
    thread_loop_info_.clear();
    std::lock_guard w(global_loops_mutex_);
    global_loops_.erase(
        std::remove(global_loops_.begin(), global_loops_.end(), this),
        global_loops_.end());
};
void EventLoop::make_current() noexcept {
    std::lock_guard lock(thread_info_mutex_);
    {
        std::lock_guard w(thread_loops_mutex_);
        if (!thread_loops_.empty() && thread_loops_.back() == this) return;
        thread_loops_.emplace_back(this);
    }
    thread_loop_info_.emplace(&thread_loops_, &thread_loops_mutex_);
}
void EventLoop::make_global() noexcept {
    std::lock_guard w(global_loops_mutex_);
    if (!global_loops_.empty() && global_loops_.back() == this) return;
    global_loops_.emplace_back(this);
}
EventLoop* EventLoop::current() {
    return global_loops_.empty()
               ? (thread_loops_.empty() ? nullptr : thread_loops_.back())
               : global_loops_.back();
}
EventLoop* EventLoop::global_loop() {
    std::shared_lock<std::shared_mutex> lock(thread_loops_mutex_);
    return global_loops_.empty() ? nullptr : global_loops_.back();
}
EventLoop* EventLoop::thread_loop() {
    std::shared_lock<std::shared_mutex> lock(thread_loops_mutex_);
    return thread_loops_.empty() ? nullptr : thread_loops_.back();
}
AbsEvent::~AbsEvent() {};
LoopGuard::LoopGuard(EventLoop& loop)
    : lock(loop.thread_loops_mutex_, std::try_to_lock) {
    if (lock.owns_lock()) EventLoop::thread_loops_.emplace_back(&loop);
    XC_ASSERT(lock.owns_lock() && "LoopGuard lock failed");
}
