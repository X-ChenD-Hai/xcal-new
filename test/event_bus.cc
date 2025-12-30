#include <gtest/gtest.h>

#include <chrono>
#include <ecs/event_bus.hpp>
#include <type_map.hpp>
#include <xc_assert.hpp>
using namespace ecs;

struct Event {
    std::string name_;
    explicit Event(std::string name) : name_(std::move(name)) {
        std::cout << "Event " << name_ << " created" << std::endl;
    }
    Event(Event&& other) : name_(std::move(other.name_)) {
        std::cout << "Event " << this << " move created name " << name_
                  << std::endl;
    }
    ~Event() {
        std::cout << "Event " << this << " name " << name_ << " destroyed"
                  << std::endl;
    }
};

struct Event2 {
    int a;
    int b;
    explicit Event2(int a, int b) : a(a), b(b) {
        std::cout << "Event2 " << this << " created" << std::endl;
    }
    Event2(Event2&& other) : a(other.a) {
        std::cout << "Event2 " << this << " move created " << std::endl;
    }
    ~Event2() { std::cout << "Event2 " << this << " destroyed" << std::endl; }
};
TEST(EventBusTest, Test1) {
    EventBus bus;

    // sizeof(Event);

    bus.publish<Event>("e 1");
    bus.publish<Event>("e 2");
    bus.publish<Event>("e 3");
    bus.publish<Event>("e 4");
    bus.publish<Event>("e 5");
    bus.publish<Event>("e 6");
    bus.publish<Event>("e 7");
    bus.publish<Event>("e 8");
    bus.publish<Event>("e 9");
    bus.publish<Event>("e 10");
    std::cout << "size " << bus.size() << std::endl;
    // for (auto& e : bus.each<Event>()) {
    //     std::cout << e.name_ << std::endl;
    // }
    bus.each([](Event& e) {
        std::cout << e.name_ << std::endl;
        return true;
    });
    std::cout << "size " << bus.size() << std::endl;
    bus.each([](Event& e) { std::cout << e.name_ << std::endl; });

    bus.clear<Event>();

    bus.publish<Event2>(1, 2);
    bus.publish<Event2>(3, 4);
    bus.publish<Event2>(5, 6);
    bus.publish<Event2>(7, 8);
    bus.publish<Event2>(9, 10);
    bus.publish<Event2>(11, 12);
    bus.publish<Event2>(13, 14);
    bus.publish<Event2>(15, 16);
    bus.publish<Event2>(17, 18);
    bus.publish<Event2>(19, 20);

    EXPECT_EQ((size_t)(bus.pool<0>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<1>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<2>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<3>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<4>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<5>().cells()) % 64, 0);
    EXPECT_EQ((size_t)(bus.pool<6>().cells()) % 64, 0);
    std::cout << "size " << bus.size() << std::endl;
    bus.clear<Event2>();
    std::cout << "size " << bus.size() << std::endl;
}

TEST(EventBusTest, swap) {
    EventBus bus1;
    bus1.publish<Event>("e 1");
    bus1.publish<Event>("e 2");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    bus1.publish<Event>("e 3");
    int a = 0;
    bus1.each([&](Event&) { return a++ > 3; });
    std::cout << "size " << bus1.size() << " pool size "
              << bus1.pool_of<Event>().pool_size() << std::endl;
    bus1.shrink();
    std::cout << "size " << bus1.size() << " pool size "
              << bus1.pool_of<Event>().pool_size() << std::endl;
}
TEST(EventBusTest, BasicPublishAndIteration) {
    EventBus bus;

    // 发布多个事件
    bus.publish<Event>("event1");
    bus.publish<Event>("event2");
    bus.publish<Event>("event3");

    // 验证事件数量
    EXPECT_EQ(bus.size<Event>(), 3);
    EXPECT_EQ(bus.size(), 3);

    // 使用lambda遍历
    int count = 0;
    bus.each([&count](const Event& e) {
        count++;
        EXPECT_TRUE(e.name_.find("event") != std::string::npos);
    });
    EXPECT_EQ(count, 3);

    // 验证事件存在性
    EXPECT_TRUE(bus.exist<Event>());
    EXPECT_TRUE(bus.all_exist<Event>());
    EXPECT_FALSE(bus.exist<Event2>());
}
TEST(EventBusTest, MultipleEventTypes) {
    EventBus bus;

    // 发布两种不同类型的事件
    bus.publish<Event>("test_event");
    bus.publish<Event2>(42, 100);
    bus.publish<Event>("another_event");

    // 验证各自的数量
    EXPECT_EQ(bus.size<Event>(), 2);
    EXPECT_EQ(bus.size<Event2>(), 1);
    EXPECT_EQ(bus.size(), 3);

    // 验证存在性检查
    EXPECT_TRUE((bus.any_exist<Event, Event2>()));
    EXPECT_TRUE((bus.all_exist<Event, Event2>()));
}
TEST(EventBusTest, MemoryPoolExpansion) {
    EventBus bus;

    // 获取初始池大小
    auto& pool = bus.pool_of<Event>();
    size_t initial_pool_size = pool.pool_size();

    // 发布超过初始容量的事件
    const int num_events = 20;
    for (int i = 0; i < num_events; ++i) {
        bus.publish<Event>("event_" + std::to_string(i));
    }

    // 验证池已扩展
    EXPECT_GT(pool.pool_size(), initial_pool_size);
    EXPECT_EQ(bus.size<Event>(), num_events);

    // 验证所有事件都可访问
    std::set<std::string> event_names;
    bus.each([&event_names](const Event& e) { event_names.insert(e.name_); });

    EXPECT_EQ(event_names.size(), num_events);
}

TEST(EventBusTest, MemoryPoolShrinking) {
    EventBus bus;

    // 发布大量事件
    const int initial_events = 50;
    for (int i = 0; i < initial_events; ++i) {
        bus.publish<Event>("event_" + std::to_string(i));
    }

    auto& pool = bus.pool_of<Event>();
    size_t pool_size_before_clear = pool.pool_size();

    // 清除大部分事件，只保留少量
    int kept = 0;
    bus.each([&kept](Event& e) {
        if (kept < 5) {
            kept++;
            return false;  // 保留
        }
        return true;  // 删除
    });

    // 手动触发收缩
    bus.shrink();

    // 验证池大小减小（注意：收缩是有条件的）
    if (bus.size<Event>() < pool_size_before_clear / 4) {
        EXPECT_LT(pool.pool_size(), pool_size_before_clear);
    }

    EXPECT_EQ(bus.size<Event>(), 5);
}
TEST(EventBusTest, RangeBasedForLoop) {
    EventBus bus;

    // 发布事件
    std::vector<std::string> expected_names = {"a", "b", "c", "d"};
    for (const auto& name : expected_names) {
        bus.publish<Event>(name);
    }

    // 使用基于范围的for循环
    std::vector<std::string> actual_names;
    for (auto& event : bus.each<Event>()) {
        actual_names.push_back(event.name_);
    }

    // 验证顺序和内容
    EXPECT_EQ(actual_names.size(), expected_names.size());
    for (size_t i = 0; i < expected_names.size(); ++i) {
        EXPECT_EQ(actual_names[i], expected_names[i]);
    }
}

TEST(EventBusTest, IteratorOperations) {
    EventBus bus;

    bus.publish<Event>("first");
    bus.publish<Event>("second");
    bus.publish<Event>("third");

    auto range = bus.each<Event>();
    auto it = range.begin();
    auto end = range.end();

    // 测试解引用
    EXPECT_FALSE(it == end);
    EXPECT_EQ((*it).name_, "first");
    EXPECT_EQ(it->name_, "first");

    // 测试前缀递增
    ++it;
    EXPECT_EQ(it->name_, "second");

    // 测试后缀递增
    it++;
    EXPECT_EQ(it->name_, "third");

    // 测试结束迭代器
    ++it;
    EXPECT_TRUE(it == end);
}

TEST(EventBusTest, EmptyBusAndClearOperations) {
    EventBus bus;

    // 测试空总线
    EXPECT_EQ(bus.size(), 0);
    EXPECT_FALSE(bus.exist<Event>());
    EXPECT_FALSE((bus.any_exist<Event, Event2>()));

    // 对空总线调用each应该返回false
    bool callback_called = false;
    bool result =
        bus.each([&callback_called](const Event&) { callback_called = true; });
    EXPECT_FALSE(result);
    EXPECT_FALSE(callback_called);

    // 发布然后清除
    bus.publish<Event>("test");
    EXPECT_TRUE(bus.exist<Event>());

    bus.clear<Event>();
    EXPECT_FALSE(bus.exist<Event>());
    EXPECT_EQ(bus.size(), 0);

    // 测试全局清除
    bus.publish<Event>("e1");
    bus.publish<Event2>(1, 2);
    EXPECT_EQ(bus.size(), 2);

    bus.clear();
    EXPECT_EQ(bus.size(), 0);
}

TEST(EventBusTest, MemoryAlignment) {
    EventBus bus;

    // 测试所有内存池都是64字节对齐的
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<0>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<1>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<2>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<3>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<4>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<5>().cells()) % 64, 0);
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool<6>().cells()) % 64, 0);

    // 发布事件后验证对齐仍然保持
    bus.publish<Event>("alignment_test");
    EXPECT_EQ(reinterpret_cast<size_t>(bus.pool_of<Event>().cells()) % 64, 0);
}

TEST(EventBusTest, LargeNumberOfEvents) {
    EventBus bus;
    const int num_events = 1000;

    // 发布大量事件
    for (int i = 0; i < num_events; ++i) {
        bus.publish<Event>("event_" + std::to_string(i));
    }

    // 验证数量
    EXPECT_EQ(bus.size<Event>(), num_events);

    // 验证所有事件都可访问
    std::vector<int> found_indices(num_events, 0);
    bus.each([&found_indices](const Event& e) {
        size_t pos = e.name_.find("_");
        if (pos != std::string::npos) {
            int index = std::stoi(e.name_.substr(pos + 1));
            if (index >= 0 && index < num_events) {
                found_indices[index]++;
            }
        }
    });

    // 每个索引应该恰好出现一次
    for (int i = 0; i < num_events; ++i) {
        EXPECT_EQ(found_indices[i], 1);
    }
}
TEST(EventBusTest, MoveSemantics) {
    EventBus bus;

    // 测试移动构造的事件
    std::string original_name = "original";
    bus.publish<Event>(std::move(original_name));

    // original_name 可能被移动（标准未指定但通常为空）
    bus.each([](const Event& e) { EXPECT_EQ(e.name_, "original"); });
}

TEST(EventBusTest, RealWorldScenario) {
    EventBus bus;

    // 模拟游戏或UI系统中的事件流
    struct MouseEvent {
        int x, y;
        bool pressed;
    };
    struct KeyEvent {
        int key;
        bool down;
    };
    struct NetworkEvent {
        std::string data;
    };

    // 发布各种事件
    bus.publish<MouseEvent>(100, 200, true);
    bus.publish<KeyEvent>(65, true);  // A键按下
    bus.publish<NetworkEvent>("received_data");
    bus.publish<MouseEvent>(150, 250, false);
    bus.publish<KeyEvent>(65, false);  // A键释放

    // 处理事件
    int mouse_events_processed = 0;
    int key_events_processed = 0;
    int network_events_processed = 0;

    bus.each<MouseEvent>([&mouse_events_processed](const MouseEvent& e) {
        mouse_events_processed++;
        EXPECT_GE(e.x, 0);
        EXPECT_GE(e.y, 0);
    });

    bus.each<KeyEvent>([&key_events_processed](const KeyEvent& e) {
        key_events_processed++;
        EXPECT_GT(e.key, 0);
    });

    bus.each<NetworkEvent>([&network_events_processed](const NetworkEvent& e) {
        network_events_processed++;
        EXPECT_FALSE(e.data.empty());
    });

    EXPECT_EQ(mouse_events_processed, 2);
    EXPECT_EQ(key_events_processed, 2);
    EXPECT_EQ(network_events_processed, 1);

    // 清除已处理的事件
    bus.clear<MouseEvent>();
    bus.clear<KeyEvent>();
    bus.clear<NetworkEvent>();

    EXPECT_EQ(bus.size(), 0);
}

// 测试事件类型
struct SimpleEvent {
    int id;
    float data[4];
    SimpleEvent(int i) : id(i) {
        for (int j = 0; j < 4; ++j) data[j] = i * 0.1f;
    }
};

// 性能测试工具宏
#define BENCHMARK_SCOPE(name)                                               \
    auto start = std::chrono::high_resolution_clock::now();                 \
    std::cout << "[" << name << "] ";                                       \
    auto _scope_guard = gtest_bench::make_scope_guard([&]() {               \
        auto end = std::chrono::high_resolution_clock::now();               \
        std::cout << "Time: "                                               \
                  << std::chrono::duration_cast<std::chrono::microseconds>( \
                         end - start)                                       \
                         .count()                                           \
                  << "μs\n";                                                \
    })

namespace gtest_bench {
struct ScopeGuard {
    std::function<void()> func;
    ~ScopeGuard() { func(); }
};
auto make_scope_guard(std::function<void()> f) { return ScopeGuard{f}; }
}  // namespace gtest_bench

// 测试用例：EventBus vs new/delete
TEST(PerformanceTest, EventBusVsNewDelete) {
    constexpr int kNumEvents = 100'000;

    // 场景1：事件创建速度
    {
        BENCHMARK_SCOPE("EventBus-Publish");
        EventBus bus;
        for (int i = 0; i < kNumEvents; ++i) {
            bus.publish<SimpleEvent>(i);
        }
    }

    {
        BENCHMARK_SCOPE("Traditional-New");
        std::vector<SimpleEvent*> events;
        events.reserve(kNumEvents);
        for (int i = 0; i < kNumEvents; ++i) {
            events.push_back(new SimpleEvent(i));
        }
        // 清理（不计入耗时）
        for (auto* e : events) delete e;
    }

    // 场景2：事件遍历处理
    {
        EventBus bus;
        for (int i = 0; i < kNumEvents; ++i) {
            bus.publish<SimpleEvent>(i);
        }

        BENCHMARK_SCOPE("EventBus-Iterate");
        int sum = 0;
        bus.each([&sum](const SimpleEvent& e) { sum += e.id; });
    }

    {
        std::vector<SimpleEvent*> events;
        for (int i = 0; i < kNumEvents; ++i) {
            events.push_back(new SimpleEvent(i));
        }

        BENCHMARK_SCOPE("Traditional-Iterate");
        int sum = 0;
        for (const auto* e : events) {
            sum += e->id;
        }

        for (auto* e : events) delete e;
    }

    // 场景3：内存池扩容压力测试
    {
        BENCHMARK_SCOPE("EventBus-PoolGrowth");
        EventBus bus;
        for (int i = 0; i < kNumEvents * 10; ++i) {  // 故意制造扩容
            bus.publish<SimpleEvent>(i);
            if (i % 10'000 == 0) bus.clear<SimpleEvent>();  // 周期性清空
        }
    }
}
