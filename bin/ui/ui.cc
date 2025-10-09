#include <event/event.hpp>
#include <print>
#include <ui/GlfwImguiWindow.hpp>

void editor_fn(std::string name, int value) {
    std::println("editor_fn called with name: {}, value: {}", name, value);
}

int main() {
    EventLoop loop;
    loop.make_global();

    Window window;

    window.add_editor(editor_fn, "editor", "name", "aa", "value", 1);
    window
        .add_editor([&](int a) { std::println("lambda called with a: {}", a); },
                    "editor", "a", 11)
        ->set_field(0, 22);

    window.show();

    return 0;
}
