#include <iostream>
#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/types.hpp>

struct MyEvent {
    int value;
};
using Call = void (*)(MyEvent&);
int main(int argc, char* argv[]) {
    ecs::EventBus event_bus;

    event_bus.publish<MyEvent>(11);

    event_bus.each(
        [](MyEvent& event, int a) {
            std::cout << "MyEvent value: " << event.value + a << std::endl;
        },
        12);
    std::cout << "size " << event_bus.size<MyEvent>() << std::endl;
    int s = 11;
    event_bus.each(
        [&](MyEvent& event, int& a) {
            std::cout << "MyEvent value: " << event.value + a << std::endl;
            return true;
        },
        s);
    std::cout << "size " << event_bus.size<MyEvent>() << std::endl;
    event_bus.clear();

    return 0;
}
