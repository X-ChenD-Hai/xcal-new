#include <cstddef>
#include <cstdint>
#include <vector>
#include <xc/async/scheduler.hpp>
#include <xc/ecs2/schedule.hpp>

#include "xc/async/async_primitives.hpp"
#include "xc/async/channel.hpp"
#include "xc/async/structure/consume_token.hpp"

namespace xc::ecs_executor {
using async::Channel;
using async::Future;
using async::structure::ConsumeToken;
using ecs::Schedule;
using channel_t = Channel<uint32_t, 32>;
using consume_token_t = ConsumeToken<>;

struct WorkerContext {
    Schedule* schedule;
    channel_t id_channel;
    channel_t finish_channel;
};

inline Future<> worker(WorkerContext& ctx, size_t id) {
    while (true) {
        auto id = co_await ctx.id_channel.recv();
        if (!id.has_value()) break;
        ctx.schedule->exec_system(id.value());
        co_await ctx.finish_channel.send(id.value());
    }
}

inline Future<> master(WorkerContext& ctx) {
    auto phase = ctx.schedule->raw_phases();
    for (auto& ids : phase) {
        uint32_t all = ids.size();
        for (auto id : ids) {
            co_await ctx.id_channel.send(id);
        }
        if (!all) break;
        while (all--) {
            co_await ctx.finish_channel.recv();
        }
    }
    ctx.id_channel.close();
    ctx.finish_channel.close();
}

inline Future<> update(Schedule& schedule, size_t max_worker_count = 16) {
    auto ctx = WorkerContext{
        .schedule = &schedule, .id_channel = {}, .finish_channel = {}};

    std::vector<Future<>> futures;
    for (size_t i = 0; i < max_worker_count; ++i) {
        futures.push_back(worker(ctx, i));
    }
    futures.push_back(master(ctx));
    co_await async::WhenAll{futures};
}

}  // namespace xc::ecs_executor