#pragma once
#include <cassert>
#include <chrono>
#include <variant>

#include "config.hpp"

namespace xc::ecs {
class SystemScheduler;
class System;
class SystemPromise;
class Worker;
class FuncTask;
struct ResumeUntilOnceTask;
struct ResumeUntilDoneTask;

class BasePromise;

using task_t = std::variant<FuncTask, ResumeUntilOnceTask, ResumeUntilDoneTask>;
using time_point_t = std::chrono::high_resolution_clock::time_point;
using time_duration_t = time_point_t::clock::duration;

}  // namespace xc::ecs