#pragma once
#include <cassert>
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

}  // namespace xc::ecs