#pragma once

#include <coroutine>
#include <functional>
namespace xc::ecs {
class SystemScheduler;
class System;
class SystemPromise;
using task_t = std::function<void(void)>;
using system_handle_t = std::coroutine_handle<SystemPromise>;
}  // namespace xc::ecs