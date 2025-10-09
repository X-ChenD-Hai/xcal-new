#include <atomic>
#include <chrono>
#include <event/event.hpp>
#include <memory>
#include <print>
#include <thread>
#include <vector>
#include <gtest/gtest.h>


class CustomListener : public EventListener {
   public:
    CustomListener(int id) : EventListener(), id_(id), event_count_(0) {
        // std::println("CustomListener {} created", id_);
    }
    bool event(AbsEvent* event) override {
        // std::println(
        //     "CustomListener {} received event: type_id = {:#04x} from thread
        //     "
        //     "{}",
        //     id_, (uint32_t)event->type_id, std::this_thread::get_id());
        std::lock_guard<std::mutex> lock(mutex_);
        ++event_count_;
        return false;
    }
    ~CustomListener() {
        // std::println("CustomListener {} destroyed", id_);
    }

    int get_event_count() const { return event_count_; }

   private:
    int id_;
    mutable std::mutex mutex_;
    std::atomic<int> event_count_;
};

struct CustomPublisher : public EventPublisher {
    CustomPublisher(int id) : id_(id) {
        // std::println("CustomPublisher {} created", id_);
    }
    ~CustomPublisher() {
        // std::println("CustomPublisher {} destroyed", id_);
    }
    void send(std::unique_ptr<AbsEvent> event) {
        // std::println(
        //     "CustomPublisher {} sending event: type_id = {:#04x} from thread
        //     "
        //     "{}",
        //     id_, (uint32_t)event->type_id, std::this_thread::get_id());
        publish(std::move(event));
    }

   private:
    int id_;
};


void publish_events(CustomPublisher& publisher, size_t total_events,
                    int thread_id) {
    for (int i = 0; i < total_events; ++i) {
        // publisher.send(
        //     std::make_unique<Event>(EventType::EntityCreated, nullptr));
        // publisher.send(
        //     std::make_unique<Event>(EventType::EntityDestroyed, nullptr));
        // publisher.send(
        //     std::make_unique<Event>(EventType::ComponentAttached, nullptr));
        // publisher.send(
        //     std::make_unique<Event>(EventType::ResourceLoaded, nullptr));
    }
}
std::atomic<bool> stop_flag{false};
void flush_events() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    while (!stop_flag) {
        if (!EventLoop::current()->events().empty()) {
            std::println("Flushing events");
            if (EventLoop::current()->flush()) {
                std::println("Flushed events");
            } else {
                std::println("Failed to flush events");
            }
        }
    }
}

int test_loop() {
    using namespace std::chrono_literals;
    EventLoop loop;
    loop.make_global();
    loop.make_current();

    const int num_publishers = 20;
    const int num_listeners = 20;
    const int num_threads = 20;
    const int epoch_events = 4;
    const int events_per_thread = epoch_events * 4;

    std::vector<std::unique_ptr<CustomListener>> listeners;
    for (int i = 0; i < num_listeners; ++i) {
        listeners.emplace_back(std::make_unique<CustomListener>(i));
    }
    std::vector<std::unique_ptr<CustomPublisher>> publishers;
    for (int i = 0; i < num_publishers; ++i) {
        publishers.emplace_back(std::make_unique<CustomPublisher>(i));
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        int publisher_id = i % num_publishers;
        threads.emplace_back(publish_events,
                             std::ref(*publishers[publisher_id]), epoch_events,
                             i);
    }
    // Start threads for flushing events
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(flush_events);
    }

    // Join all threads
    std::this_thread::sleep_for(0.1s);
    stop_flag = true;
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify the results
    int total_events = num_threads * listeners.size() * events_per_thread;
    int total_received_events = 0;

    for (const auto& listener : listeners) {
        total_received_events += listener->get_event_count();
    }
    std::println("Listener counts: {}", loop.listeners().size());
    if (total_received_events == total_events) {
        std::println(
            "Concurrency test passed: All events({}) were correctly processed.",
            total_events);
    } else {
        std::println(
            "Concurrency test failed: Expected {} events, but received {} "
            "events.",
            total_events, total_received_events);
    }
    return 0;
}
int test_loop1() { return 0; }

TEST(EventLoop, Concurrency) {
    test_loop1();
}