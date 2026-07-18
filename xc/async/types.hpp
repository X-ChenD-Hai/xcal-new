#pragma once
#include <cassert>
#include <chrono>
#include <variant>

namespace xc::async {
class SystemScheduler;
class System;
class SystemPromise;
class Worker;
class FuncTask;
struct ResumeUntilOnceTask;
struct AsyncEndWaitTask;
template <typename Task>
class CancelAbleTask;
class BasePromise;
class CancelTask;
class CancelToken;
class CancelWithRollbackTask;

using task_t =
    std::variant<FuncTask, ResumeUntilOnceTask, AsyncEndWaitTask, CancelTask,
                 CancelWithRollbackTask, CancelAbleTask<FuncTask>,
                 CancelAbleTask<ResumeUntilOnceTask>,
                 CancelAbleTask<AsyncEndWaitTask>, CancelAbleTask<CancelTask>,
                 CancelWithRollbackTask>;
using time_point_t = std::chrono::high_resolution_clock::time_point;
using time_duration_t = time_point_t::clock::duration;

}  // namespace xc::async