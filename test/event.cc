#include <gtest/gtest.h>
#include <print>
#include <chrono>
#include <event/event.hpp>
#include <event/timer.hpp>
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
class CustomListener : public EventListener {
    CTimer timer_;
    size_t timeout_count = 0;
    size_t frame_count = 0;
    size_t tick_duration = 500;
    float fps = 0.0f;
    size_t stop_after_count = 5;

   public:
    CustomListener(int id) : EventListener(), id_(id), event_count_(0) {
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
        CustomListener listener1(1);
        CustomListener listener2(2);
        CustomListener listener3(3);
        CustomListener listener4(4);

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
