#include <xc/event/event.hpp>
#include <print>
#include <xc/ui/scene_window.hpp>

static void editor_fn(std::string name, int value) {
    std::println("editor_fn called with name: {}, value: {}", name, value);
}

int main() {
    EventLoop loop;
    loop.make_global();

    SceneWindow window{"Scene", 1280, 720, 144};

    window.show();

    return 0;
}
