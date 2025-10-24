#include <gtest/gtest.h>

#include <chrono>
#include <event/event.hpp>
#include <event/timer.hpp>
#include <print>

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

TEST(EventLoop, Concurrency) { test_loop1(); }

std::mutex global_loops_mutex;
std::atomic<bool> stop_loop = false;
std::condition_variable stop_loop_cv;
enum class EventType : uint32_t {
    //
    Update = 1 << 0,
    TimeOut,
    EntityCreated = Update << 2,
    EntityDestroyed,
    //
    ComponentAttached = EntityCreated << 1,
    ComponentDetached,
    ComponentUpdated,
    //
    ResourceLoaded = ComponentAttached << 1,

    //
    All = ~(uint32_t(1 << 31)),
};
class Event : public AbsEvent {
    EventType type_id;
    void* sender_;

   public:
    EventType type() const { return type_id; }
    void* sender() const { return sender_; }
    Event(EventType type_id, void* sender)
        : type_id(type_id), sender_(sender) {}
    ~Event() override {}
};

using Cfg =
    EventConfig<Event, EventType, EventType::TimeOut, EventType::Update>;

using CTimer = Timer<Cfg, [](void* self) -> std::unique_ptr<AbsEvent> {
    return std::make_unique<Event>(EventType::TimeOut, self);
}>;
class CustomListener2 : public EventListener {
    CTimer timer_;
    size_t timeout_count = 0;
    size_t frame_count = 0;
    size_t tick_duration = 500;
    float fps = 0.0f;
    size_t stop_after_count = 5;

   public:
    CustomListener2(int id) : EventListener(), id_(id), event_count_(0) {
        using namespace std::chrono_literals;
        // std::println("CustomListener {} created", id_);
        timer_.start(std::chrono::milliseconds(tick_duration), true);
    }
    bool event(AbsEvent* event) override {
        auto e = static_cast<Event*>(event);
        if (e->type() == EventType::TimeOut && e->sender() == &timer_) {
            if (++timeout_count >= stop_after_count) stop_loop = true;
            fps = (float)frame_count / (float)tick_duration * 1000.0f;
            std::println(
                "FPS = {:.2f}  CustomListener {} received TimeOut event, count "
                "= {}",
                fps, id_, timeout_count);
            frame_count = 0;
            return true;
        }
        if (e->type() == EventType::Update) {
            frame_count++;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        ++event_count_;
        return false;
    }
    ~CustomListener2() {
        // std::println("CustomListener {} destroyed", id_);
    }

    int get_event_count() const { return event_count_; }

   private:
    int id_;
    mutable std::mutex mutex_;
    std::atomic<int> event_count_;
};

struct CustomPublisher2 : public EventPublisher {
    CustomPublisher2(int id) : id_(id) {
        // std::println("CustomPublisher {} created", id_);
    }
    ~CustomPublisher2() {
        // std::println("CustomPublisher {} destroyed", id_);
    }
    void send(std::unique_ptr<Event> event) {
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

TEST(EventTest, Test1) {
    using namespace std::chrono_literals;

    EventLoop loop;
    loop.make_global();
    std::thread t1([&]() {
        CustomListener2 listener1(1);
        CustomListener2 listener2(2);
        CustomListener2 listener3(3);
        CustomListener2 listener4(4);

        while (!stop_loop) {
            loop.publish(std::make_unique<Event>(EventType::Update, &loop));
            loop.flush();
        }
    });
    std::thread t2([&]() {

    });
    t1.join();
    t2.join();
}
