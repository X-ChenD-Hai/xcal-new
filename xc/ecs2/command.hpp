#include "xc/ecs2/comman/traits.hpp"

namespace xc::ecs {
template <typename... T>
struct Create;
template <typename... T>
struct Destroy;
template <typename... T>
struct Attach;
template <typename... T>
struct Detach;

template <typename... T>
class CommandQueue;
template <typename... T>
class CommandQueue<Create<T...>> {
   public:
   private:
};
}  // namespace xc::ecs