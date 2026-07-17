#include <float.h>

#include <cstddef>
#include <print>
#include <xc/ecs2/async_primitives.hpp>
#include <xc/ecs2/scheduler.hpp>
#include <xc/ecs2/types.hpp>
#include <xc/ecs2/utility.hpp>

#include "xc/ecs2/structure/consume_token.hpp"

using namespace xc::ecs;
using namespace xc::ecs::structure;
Future<> sys1(SyncToken& tk, structure::ConsumeToken<> st) {
    std::println("sys1 start");
    auto pointer = tk.pointer();
    while (st.remaining()) {
        auto d = co_await pointer->next();
        std::println("sys1 next {}", d);
    }
    std::println("sys1 exit");
    co_return;
}
Future<> sys2(SyncToken& tk, structure::ConsumeToken<> st) {
    using namespace std::chrono_literals;
    std::println("sys2 start");
    auto pointer = tk.pointer();
    while (st.remaining()) {
        auto d = co_await pointer->next();
        co_await Sleep{200ms};
        std::println("sys2 next {}", d);
        if (d > 1) {
            pointer->sync_tick();
        }
    }
    std::println("sys2 exit");
    co_return;
}

Future<> sys_main(SyncToken& tk, structure::ConsumeToken<> st) {
    std::println("sys_main");
    using namespace std::chrono_literals;
    size_t expected = 100;
    while (expected--) {
        co_await Sleep{100ms};
        std::println("sys_main_step");
        tk.step();
    }
    st.consume_all();
    std::println("sys_main consume_all");
    co_await tk.expect_zero_pointer();
    std::println("sys_main exit");
    co_return;
}

System test_sync() {
    std::println("test_sync");
    SyncToken tk{};
    structure::ConsumeToken<> st{1};

    co_await (sys_main(tk, st) && sys1(tk, st) && sys2(tk, st));

    co_return;
}

int main(int argc, char* argv[]) {
    SystemScheduler scheduler{};
    scheduler.start_workers();
    scheduler.add_system(test_sync());
    scheduler.update();
    scheduler.stop_workers();
    return 0;
}
