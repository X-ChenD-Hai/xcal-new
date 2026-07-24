#include "xc/ecs2/comman/traits.hpp"
#include "xc/ecs2/markers.hpp"
namespace xc::ecs {

template <typename... T>
class Context;

template <typename... T>
using base_context_t =
    collect_all_markers_t<traits::template_record<Base>, T...>;
template <typename T>
class Context<Base<T>> : public T {};
template <>
class Context<Base<>> {};

}  // namespace xc::ecs