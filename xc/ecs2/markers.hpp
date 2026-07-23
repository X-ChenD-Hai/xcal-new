#pragma once
#include "xc/ecs2/comman/traits.hpp"
namespace xc::ecs {
template <typename... T>
class Derived;

template <typename... T>
class Resource;

template <typename T>
class Optional {
    using type = T;
};

template <typename... Eany>
struct ExcludeAny;

template <typename... Eall>
struct ExcludeAll;

template <typename... R>
struct Read;

template <typename... Rw>
struct ReadWrite;

template <typename... Rw>
struct CacheTag;

template <typename... T>
struct CreateEntity;

template <typename... T>
struct DestroyEntity;

template <typename... T>
struct Attach;

template <typename... T>
struct Detach;

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... T>
struct collect_marker;

template <template <typename...> typename marker, typename exclude_record,
          typename... T>
struct collect_marker<true, marker, exclude_record, traits::type_record<T...>>
    : collect_marker<true, marker, exclude_record, T...> {};
template <template <typename...> typename marker, typename exclude_record,
          typename... T>
struct collect_marker<false, marker, exclude_record, traits::type_record<T...>>
    : collect_marker<false, marker, exclude_record, T...> {};

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... T>
using collect_marker_t =
    traits::deref<collect_marker<inc_unwrapper, marker, exclude_record, T...>>;

template <template <typename...> typename marker, typename exclude_record,
          typename... T>
struct collect_marker<false, marker, exclude_record, T...>
    : traits::return_type<traits::flatten_t<traits::filter_if_t<
          marker<T...>,
          traits::conjunction<
              traits::is_specialized_from<marker>,
              traits::repack_template_t<exclude_record,
                                        traits::not_specialized_from>>>>> {};
template <template <typename...> typename marker, typename exclude_record,
          typename... T>
struct collect_marker<true, marker, exclude_record, T...>
    : traits::return_type<traits::flatten_t<traits::filter_if_t<
          marker<T...>, traits::repack_template_t<
                            exclude_record, traits::not_specialized_from>>>> {};
};  // namespace xc::ecs