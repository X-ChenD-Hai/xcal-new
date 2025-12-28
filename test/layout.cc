#include <gtest/gtest.h>

#include <cstdint>
#include <print>

enum class CommandType {
    Sync,
    Async,
};

struct Command {
    // command type (Sync|Async)
    // command promise
    // command execuater
    // command result
};

struct CommandBuffer {
    // execute command which type is Sync and store result as a promise
    // command set -> impl command
    // command pipeline
    // parallel execute command in a command set
    // serial execute command in a pipeline
};

// World is a container
//  -- framecache
//      -- query_cache
//  -- entity set
//  -- component pool
// create entity
// destroy entity
// attach component to entity
// deatach comp from entity
// querry (c to e),(e to c)
// iter c (read some and change some),(read some),(change some)
//      read sync && change async

struct Destribtion {};

struct ObjecttionBuilder {};  // -> build(desc) -> entity

struct ObjectBuildCommand {};

struct ObjectEntity {
    // vaild :bool
};

using entity_t = uint32_t;
template <typename ComponentPool>
concept ComponentPoolConcept = requires(ComponentPool pool) {
    { pool.at() };
};

class CommandBuffer;
class ComponentPool;
class QueryCacher;
template <typename CommandBuffer = CommandBuffer,
          typename ComponentPool = ComponentPool,
          typename QueryCacher = QueryCacher>
class World {
   public:
   private:
    CommandBuffer command_buffer_{this};
    ComponentPool component_pool_{this};
    QueryCacher query_cacher_{this};
};

TEST(Layout, layout) { std::println("Hello world!"); }
