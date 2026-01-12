#define DEBUG(...) std::println(__VA_ARGS__)
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