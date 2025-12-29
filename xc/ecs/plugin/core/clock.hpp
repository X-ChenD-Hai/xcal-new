#pragma once
#include <chrono>
#include <ecs/command/command.hpp>
#include <ecs/command_submit.hpp>
#include <queue>
#include <vector>
namespace ecs {
class World;
namespace core {
class Clock {
    friend class ::ecs::World;
    template <typename Tp>
    using less_queue =
        std::priority_queue<Tp, std::vector<Tp>, std::greater<Tp>>;
    using time_point = std::chrono::high_resolution_clock::time_point;
    using time_duration = std::chrono::duration<double>;
    struct TimeOutTask {
        double until;
        mutable std::unique_ptr<ecs::command::Command> command;
        auto operator<=>(const TimeOutTask& t) const noexcept {
            return until <=> t.until;
        }
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
        last_ = current_;
        current_ = std::chrono::high_resolution_clock::now();
        dt_ = current_ - last_;
        runtime_ = std::chrono::duration_cast<std::chrono::duration<double>>(
                       current_ - start_)
                       .count();
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

   private:
    time_point start_;
    time_point last_;
    time_point current_;
    std::chrono::high_resolution_clock::duration dt_;
    double runtime_;
    less_queue<TimeOutTask> timeout_tasks_;
};
}  // namespace core
}  // namespace ecs
