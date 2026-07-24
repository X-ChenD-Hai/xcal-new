#pragma once
#include <cstddef>
#include <type_traits>

#include "xc/ecs2/comman/traits.hpp"
#define MARKER(marker)       \
    template <typename... T> \
    class marker;            \
    template <typename... T> \
    constexpr size_t marker_grade<marker<T...>> = (__COUNTER__ + 1);
#define Container          \
    template <typename...> \
    typename
#define TContainer                                \
    template <template <typename...> typename...> \
    typename
namespace xc::ecs {

template <typename... T>
constexpr size_t marker_grade = 0;

MARKER(Read);
MARKER(ReadWrite);
MARKER(ExcludeAny);
MARKER(ExcludeAll);
MARKER(Optional);

MARKER(Derived);

MARKER(Resource);
MARKER(CacheTag);

MARKER(CreateEntity);
MARKER(DestroyEntity);
MARKER(Attach);
MARKER(Detach);

template <typename T, typename U>
struct marker_less
    : std::integral_constant<bool, (marker_grade<T> < marker_grade<U>)> {};

using marker_less_then = traits::bind<marker_less>;

template <typename T>
using sort_markers_t = traits::sort_t<T, marker_less_then>;

template <typename T, typename U>
using merge_markers_t =
    traits::order_preserving_merge_t<T, U, marker_less_then>;

template <typename T>
class Optional<T> {
    using type = T;
};
template <typename T, typename U, typename = void>
struct fold_marker;
template <typename T, typename U>
struct fold_marker<T, U, std::enable_if_t<traits::is_same_specialized_v<T, U>>>
    : traits::concat<T, U> {};
template <typename T, typename U>
struct fold_marker<T, U, std::enable_if_t<!traits::is_same_specialized_v<T, U>>>
    : traits::return_type<T> {};

template <typename T, typename U, typename = void>
struct fold_marker_with_default;
template <typename T, typename U>
struct fold_marker_with_default<
    T, U, std::enable_if_t<traits::is_same_specialized_v<T, U>>>
    : traits::concat<T, U> {};
template <typename T, typename U>
struct fold_marker_with_default<
    T, U,
    std::enable_if_t<!traits::is_same_specialized_v<T, U> && marker_grade<U>>>
    : traits::return_type<T> {};
template <typename T, typename U>
struct fold_marker_with_default<
    T, U,
    std::enable_if_t<!traits::is_same_specialized_v<T, U> && !marker_grade<U>>>
    : traits::insert_at<T, traits::size_of_v<T>, U> {};

template <Container marker, typename... T>
struct collect_marker;
template <Container marker, typename... T>
using collect_marker_t = traits::deref<collect_marker<marker, T...>>;
template <Container marker, typename... T>
struct collect_marker : traits::fold_left<traits::type_record<T...>, marker<>,
                                          traits::bind<fold_marker>> {};
template <Container marker, typename... T>
struct collect_marker<marker, traits::type_record<T...>>
    : collect_marker<marker, T...> {};

template <Container marker, typename... T>
struct collect_default_marker;
template <Container marker, typename... T>
using collect_default_marker_t =
    traits::deref<collect_default_marker<marker, T...>>;
template <Container marker, typename... T>
struct collect_default_marker
    : traits::fold_left<traits::type_record<T...>, marker<>,
                        traits::bind<fold_marker_with_default>> {};
template <Container marker, typename... T>
struct collect_default_marker<marker, traits::type_record<T...>>
    : collect_default_marker<marker, T...> {};

template <typename markers, typename... T>
struct collect_all_markers;
template <typename markers, typename... T>
using collect_all_markers_t = traits::deref<collect_all_markers<markers, T...>>;
template <typename markers, typename... T>
struct collect_all_markers<markers, traits::type_record<T...>>
    : collect_all_markers<markers, T...> {};
template <TContainer C, Container... M, typename... T>
struct collect_all_markers<C<M...>, T...>
    : traits::return_type<traits::type_record<collect_marker_t<M, T...>...>> {};

template <Container default_marker, typename markers, typename... T>
struct collect_all_markers_with_default;
template <Container default_marker, typename markers, typename... T>
using collect_all_markers_with_default_t = traits::deref<
    collect_all_markers_with_default<default_marker, markers, T...>>;
template <Container default_marker, typename markers, typename... T>
struct collect_all_markers_with_default<default_marker, markers,
                                        traits::type_record<T...>>
    : collect_all_markers_with_default<default_marker, markers, T...> {};
template <TContainer C, Container default_marker, Container... M, typename... T>
struct collect_all_markers_with_default<default_marker, C<M...>, T...>
    : traits::return_type<
          traits::type_record<collect_default_marker_t<default_marker, T...>,
                              collect_marker_t<M, T...>...>> {};

// template <template <typename...> typename marker, typename exclude_record,
//           typename... T>
// struct collect_marker<true, marker, exclude_record,
// traits::type_record<T...>>
//     : collect_marker<true, marker, exclude_record, T...> {};
// template <template <typename...> typename marker, typename exclude_record,
//           typename... T>
// struct collect_marker<false, marker, exclude_record,
// traits::type_record<T...>>
//     : collect_marker<false, marker, exclude_record, T...> {};

// template <bool inc_unwrapper, template <typename...> typename marker,
//           typename exclude_record, typename... T>
// using collect_marker_t =
//     traits::deref<collect_marker<inc_unwrapper, marker, exclude_record,
//     T...>>;

// template <template <typename...> typename marker, typename exclude_record,
//           typename... T>
// struct collect_marker<false, marker, exclude_record, T...>
//     : traits::return_type<traits::flatten_t<traits::filter_if_t<
//           marker<T...>,
//           traits::conjunction<
//               traits::is_specialized_from<marker>,
//               traits::repack_template_t<exclude_record,
//                                         traits::not_specialized_from>>>>> {};
// template <template <typename...> typename marker, typename exclude_record,
//           typename... T>
// struct collect_marker<true, marker, exclude_record, T...>
//     : traits::return_type<traits::flatten_t<traits::filter_if_t<
//           marker<T...>, traits::repack_template_t<
// exclude_record, traits::not_specialized_from>>>> {};

};  // namespace xc::ecs
#undef MARKER
#undef Container
#undef TContainer