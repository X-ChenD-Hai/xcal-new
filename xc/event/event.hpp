// event.hpp
#pragma once
#include <bitset>
#include <cassert>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>


using CompMask = std::bitset<64>;
template <typename _Event, typename _EventType, _EventType _TimeOut,
          _EventType _Update>
struct EventConfig {
    using Event = _Event;
    using EventType = _EventType;
    static constexpr EventType TimeOut = _TimeOut;
    static constexpr EventType Update = _Update;
};
template <typename Config>
concept EventConfigConcept = requires {
    typename Config::Event;
    typename Config::EventType;
    { Config::TimeOut } -> std::convertible_to<typename Config::EventType>;
    { Config::Update } -> std::convertible_to<typename Config::EventType>;
};

class AbsEvent {
   public:
    AbsEvent() = default;
    virtual ~AbsEvent() = 0;
};

class EventLoop;
class EventListener;
class EventListener {
   public:
    EventListener(const EventListener&) = default;
    EventListener(EventListener&&) = default;
    EventListener& operator=(const EventListener&) = default;
    EventListener& operator=(EventListener&&) = default;
    virtual bool event(AbsEvent* event) = 0;
    EventListener();
    virtual ~EventListener() = 0;
};
class EventPublisher {
    friend class EventLoop;
    EventLoop* loop_{nullptr};

   public:
    EventPublisher();
    EventPublisher(EventLoop* loop) : loop_(loop) {}
    EventLoop* loop() const { return loop_; }
    void set_loop(EventLoop* loop) { loop_ = loop; }
    void publish(std::unique_ptr<AbsEvent> event);
};

class EventLoop {
    friend class LoopGuard;

   private:
    thread_local static std::vector<EventLoop*> thread_loops_;
    thread_local static std::shared_mutex thread_loops_mutex_;
    static std::vector<EventLoop*> global_loops_;
    static std::shared_mutex global_loops_mutex_;
    std::queue<std::unique_ptr<AbsEvent>> events_{};
    std::unordered_set<EventListener*> listeners_{};
    std::unordered_set<EventListener*> removed_listeners_{};
    std::mutex listeners_mutex_{};
    std::mutex removed_listeners_mutex_{};
    std::mutex events_mutex_{};
    std::mutex sub_events_mutex_{};
    std::mutex thread_info_mutex_{};
    std::unordered_map<std::vector<EventLoop*>*, std::shared_mutex*>
        thread_loop_info_{};

   public:
    EventLoop() = default;
    ~EventLoop();
    void make_current() noexcept;
    void make_global() noexcept;
    static EventLoop* current();
    static EventLoop* global_loop();
    static EventLoop* thread_loop();
    void subscribe(EventListener* listener);
    void unsubscribe(EventListener* listener);
    bool flush() noexcept;
    void publish(std::unique_ptr<AbsEvent> event);
    const std::unordered_set<EventListener*>& listeners();
    const std::queue<std::unique_ptr<AbsEvent>>& events();
    void clear();
};
class LoopGuard {
    std::unique_lock<std::shared_mutex> lock;

   public:
    LoopGuard(EventLoop& loop);
    ~LoopGuard() { EventLoop::thread_loops_.pop_back(); }
};