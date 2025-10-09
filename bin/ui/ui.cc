#include <event/event.hpp>
#include <print>
#include <ui/SceneWindow.hpp>

void editor_fn(std::string name, int value) {
    std::println("editor_fn called with name: {}, value: {}", name, value);
}

int main() {
    EventLoop loop;
    loop.make_global();

    SceneWindow window;

    window.show();

    return 0;
}
