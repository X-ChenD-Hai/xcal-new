#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <xc/ecs/command/command.hpp>
#include <xc/ecs/command/function.hpp>
#include <xc/ecs/command_submit.hpp>

namespace ecs {
class World;
namespace core {
class Clock;

class TickTask {
    friend class Clock;

   public:
    TickTask(const TickTask&) = delete;
    TickTask(TickTask&&) = default;
    TickTask& operator=(const TickTask&) = delete;
    TickTask& operator=(TickTask&&) = delete;
    TickTask(double tick_duration, ecs::command::command_ptr&& command)
        : command_(std::move(command)), tick_duration_(tick_duration) {}

   protected:
    inline void tick(double dt, CommandSubmit& submit) {
        if (!run_flag_) return;
        runtime_ += dt;
        if (runtime_ >= tick_duration_) {
            runtime_ = 0;
            submit.submit(command_.get());
        }
    }
    inline void suspend() { run_flag_ = false; }
    inline void resume() { run_flag_ = true; }
    inline void clear_duration() { runtime_ = 0; }

   private:
    ecs::command::command_ptr command_;
    double tick_duration_;
    double runtime_{0};
    bool run_flag_{true};
};

class TickHandler {
   public:
    size_t id{std::numeric_limits<size_t>::max()};
};

class Clock {
    friend class ::ecs::World;
    friend class TickHandler;
    template <typename Tp>
    using less_queue =
        std::priority_queue<Tp, std::vector<Tp>, std::greater<Tp>>;
    using time_point = std::chrono::high_resolution_clock::time_point;
    using time_duration = std::chrono::duration<double>;
    struct TimeOutTask {
        auto operator<=>(const TimeOutTask& t) const noexcept {
            return until <=> t.until;
        }
        double until;
        mutable std::unique_ptr<ecs::command::Command> command;
    };

   public:
    void run_for(double s, std::unique_ptr<ecs::command::Command>&& cmd) {
        timeout_tasks_.emplace(s + runtime_, std::move(cmd));
    }
    void run_until(double s, std::unique_ptr<ecs::command::Command>&& cmd) {
        timeout_tasks_.emplace(s, std::move(cmd));
    }
    void run_until(time_duration duration,
                   std::unique_ptr<ecs::command::Command>&& cmd) {
        timeout_tasks_.emplace(
            std::chrono::duration_cast<time_duration>(duration).count(),
            std::move(cmd));
    }

    TickHandler tick(double dt, ecs::command::command_ptr&& task) {
        tick_tasks_.emplace_back(dt, std::move(task));
        return {tick_tasks_.size() - 1};
    }
    TickHandler tick(double dt, std::function<void()>&& task) {
        tick_tasks_.emplace_back(
            dt, std::make_unique<command::Function>(std::move(task)));
        return {tick_tasks_.size() - 1};
    }
    inline void suspend(TickHandler handler) {
        tick_tasks_[handler.id].suspend();
    }
    inline void resume(TickHandler& handler) {
        tick_tasks_[handler.id].resume();
    }
    inline void reset(TickHandler handler) {
        tick_tasks_[handler.id].clear_duration();
    }

   public:
    inline double dt_s() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<double>>(dt_)
            .count();
    }
    inline double runtime() const noexcept { return runtime_; }
    inline std::chrono::high_resolution_clock::duration dt() const noexcept {
        return dt_;
    }
    inline time_point start_time() const noexcept { return start_; }
    inline time_point current_time() const noexcept { return current_; }
    inline time_point last_time() const noexcept { return last_; }

   protected:
    Clock() {}
    inline void update_time() {
        last_ = current_;
        current_ = std::chrono::high_resolution_clock::now();
        dt_ = current_ - last_;
        runtime_ = std::chrono::duration_cast<std::chrono::duration<double>>(
                       current_ - start_)
                       .count();
    }
    inline void poll_tasks(CommandSubmit& submit) {
        auto& tasks = timeout_tasks_;
        while (!tasks.empty()) {
            auto& t = tasks.top();
            if (t.until > runtime_) {
                break;
            }
            submit.submit(std::move(t.command));
            tasks.pop();
        }
    }
    inline void pool_tick(CommandSubmit& submit) {
        for (auto& t : tick_tasks_) {
            t.tick(dt_s(), submit);
        }
    }

   protected:
    static Clock* install(World& world) {
        auto c = new Clock();
        c->runtime_ = 0;
        c->start_ = c->last_ = c->current_ =
            std::chrono::high_resolution_clock::now();
        c->dt_ = c->current_ - c->last_;

        return c;
    }
    static void uninstall(World& world, Clock* clock) { delete clock; }
    void run(ecs::CommandSubmit& submit) {
        update_time();
        poll_tasks(submit);
        pool_tick(submit);
    }

   private:
    time_point start_;
    time_point last_;
    time_point current_;
    std::chrono::high_resolution_clock::duration dt_;
    double runtime_;
    less_queue<TimeOutTask> timeout_tasks_;
    std::vector<TickTask> tick_tasks_;
};
}  // namespace core
}  // namespace ecs
