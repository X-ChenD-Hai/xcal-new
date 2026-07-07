#include <print>  // IWYU pragma: export
#define ENABLE_LOC
#ifdef ENABLE_LOC
#define LOC std::print("[{}:{}] ", __FILE__, __LINE__);
#else
#define LOC
#endif
#define DEBUG(...)                     \
    do {                               \
        LOC std::println(__VA_ARGS__); \
    } while (0)
// #define WORKER_DEBUG
// #define SCHEDULER_DEBUG

#ifdef WORKER_DEBUG
#define _WORKER_DEBUG DEBUG
#else
#define _WORKER_DEBUG(...)
#endif
#ifdef SCHEDULER_DEBUG
#define _SCHEDULER_DEBUG DEBUG
#else
#define _SCHEDULER_DEBUG(...)
#endif