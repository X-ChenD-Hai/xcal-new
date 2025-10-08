#include <print>
#include <ui/GlfwImguiWindow.hpp>
#include <event/event.hpp>

int main() {

    EventLoop loop;
    loop.make_global();

    Window window;
    window.show();

    return 0;
}
