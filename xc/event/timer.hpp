#pragma once
#include "./event.hpp"
using TimeOutEventAllocator = std::unique_ptr<AbsEvent>(void*);
template <EventConfigConcept _EventConfig, TimeOutEventAllocator _TimeOutEventAllocator>
class Timer : public EventListener, public EventPublisher {
    using Event = typename _EventConfig::Event;
    using EventType = typename _EventConfig::EventType;
   private:
    std::chrono::steady_clock::time_point until_;
    std::chrono::steady_clock::duration interval_;
    bool running_ : 1 = false;
    bool ticking_ : 1 = false;

   public:
    Timer() : EventListener(), EventPublisher() {}
    Timer(EventLoop* loop) : EventListener(), EventPublisher(loop) {}
    bool event(AbsEvent* event) override {
        if (!running_) return false;
        if (static_cast<Event*>((event))->type() != _EventConfig::Update) return false;
        auto now = std::chrono::steady_clock::now();
        if (now < until_) return false;
        this->loop()->publish(_TimeOutEventAllocator(this));
        if (!(running_ = ticking_)) return false;
        until_ = now + interval_;
        return false;
    }
    void start(std::chrono::steady_clock::duration interval,
               bool ticking = false) noexcept {
        interval_ = interval;
        running_ = true;
        ticking_ = ticking;
        until_ = std::chrono::steady_clock::now() + interval_;
    }
    void stop() noexcept {
        running_ = false;
        ticking_ = false;
    }
    void pause() noexcept {
        running_ = false;
        ticking_ = false;
    }
    void resume() noexcept {
        running_ = true;
        ticking_ = true;
        until_ = std::chrono::steady_clock::now() + interval_;
    }
};

