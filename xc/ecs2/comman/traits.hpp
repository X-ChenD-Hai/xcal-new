#pragma once
#include <cstddef>
#include <type_traits>
#define Container          \
    template <typename...> \
    class
namespace xc::traits {
using std::conditional_t;
using std::enable_if_t;
using std::false_type;
using std::integral_constant;
using std::true_type;
template <typename T>
struct return_type {
    using type = T;
};
template <typename T, T v>
struct constant_type {
    static constexpr T value = v;
    using type = T;
};
template <typename T>
using deref = typename T::type;
template <typename fn, typename... T>
struct invoke_meta;
template <typename fn, typename... T>
constexpr bool invoke_meta_v = invoke_meta<fn, T...>::value;
template <typename fn, typename... T>
using invoke_meta_t = deref<invoke_meta<fn, T...>>;

template <Container Tmp, typename... T>
struct is_specialized : false_type {};
template <Container Tmp, typename... T>
constexpr bool is_specialized_v = is_specialized<Tmp, T...>::value;
template <typename T, Container Tmp>
constexpr bool specialized_from_v = is_specialized<Tmp, T>::value;
template <Container Tmp, typename... T, typename... U>
struct is_specialized<Tmp, Tmp<T...>, U...>
    : std::conjunction<is_specialized<Tmp, U>...> {};
template <Container Tmp>
struct specialized_from;
template <typename... T, Container Tmp>
struct invoke_meta<specialized_from<Tmp>, T...> : return_type<Tmp<T...>> {};

template <Container Tmp>
struct is_specialized_from;
template <typename... T, Container Tmp>
struct invoke_meta<is_specialized_from<Tmp>, Tmp<T...>> : true_type {};
template <typename T, Container Tmp>
struct invoke_meta<is_specialized_from<Tmp>, T> : false_type {};

template <Container... T>
struct template_record;
template <template <typename> typename T>
struct predicate;

template <template <typename> typename fn, typename... T>
struct invoke_meta<template_record<fn>, T...> : fn<T...> {};
template <template <typename> typename fn, typename T>
struct invoke_meta<predicate<fn>, T> : fn<T> {};

template <typename... Pre>
struct conjunction;
template <typename... Pre>
struct disjunction;
template <typename Pre>
struct negation;

template <typename... P, typename... T>
struct invoke_meta<conjunction<P...>, T...>
    : std::conjunction<invoke_meta<P, T...>...> {};
template <typename... P, typename... T>
struct invoke_meta<disjunction<P...>, T...>
    : std::disjunction<invoke_meta<P, T...>...> {};
template <typename P, typename... T>
struct invoke_meta<negation<P>, T...> : std::negation<invoke_meta<P, T...>> {};

template <Container... Tmp>
using not_specialized_from = negation<disjunction<is_specialized_from<Tmp>...>>;

template <typename T>
struct same_as;
template <typename T, typename... Ns>
struct invoke_meta<same_as<T>, T, Ns...> : invoke_meta<same_as<T>, Ns...> {};
template <typename T>
struct invoke_meta<same_as<T>, T> : true_type {};
template <typename T, typename U>
struct invoke_meta<same_as<T>, U> : false_type {};

template <typename... T>
struct type_record;

template <typename T, typename... Ts>
struct push_front;

template <typename T, typename... Ts>
using push_front_t = deref<push_front<T, Ts...>>;

template <Container container, typename... T, typename... Ts>
struct push_front<container<Ts...>, T...>
    : return_type<container<T..., Ts...>> {};
template <typename T, Container container>
struct repack;
template <typename T, Container container>
using repack_t = deref<repack<T, container>>;
template <typename... T, Container o, Container container>
struct repack<o<T...>, container> : public return_type<container<T...>> {};
template <typename T, template <Container...> typename container>
struct repack_template;
template <typename T, template <Container...> typename container>
using repack_template_t = deref<repack_template<T, container>>;
template <Container... T, template <Container...> typename o,
          template <Container...> typename container>
struct repack_template<o<T...>, container>
    : public return_type<container<T...>> {};

template <typename container, typename predicate>
struct count_if;
template <typename container, typename predicate>
static constexpr size_t count_if_v = count_if<container, predicate>::value;
template <typename... T, Container container, typename predicate>
struct count_if<container<T...>, predicate>
    : integral_constant<size_t, (invoke_meta_v<predicate, T> + ...)> {};
template <Container container, typename predicate>
struct count_if<container<>, predicate> : integral_constant<size_t, 0> {};
template <typename container, typename predicate>
struct contains_if : integral_constant<bool, count_if_v<container, predicate>> {
};

template <typename container, typename predicate>
static constexpr bool contains_if_v = contains_if<container, predicate>::value;

template <typename T>
struct size_of;
template <typename T>
static const size_t size_of_v = size_of<T>::value;
template <Container container, typename... T>
struct size_of<container<T...>> : integral_constant<size_t, sizeof...(T)> {};

template <typename container, size_t I, typename... T>
struct insert_at;
template <typename container, size_t I, typename... T>
using insert_at_t = deref<insert_at<container, I, T...>>;
template <Container container, typename... U>
struct insert_at<container<>, 0, U...> : return_type<container<U...>> {};
template <typename T, typename... Ts, Container container, size_t I,
          typename... U>
struct insert_at<container<T, Ts...>, I, U...>
    : return_type<
          insert_at_t<insert_at_t<container<Ts...>, I - 1, U...>, 0, T>> {};
template <typename T, typename... Ts, Container container, typename... U>
struct insert_at<container<T, Ts...>, 0, U...>
    : return_type<container<U..., T, Ts...>> {};

template <typename container, size_t I>
struct remove_at;
template <typename container, size_t I>
using remove_at_t = deref<remove_at<container, I>>;
template <typename T, typename... Ts, Container container, size_t I>
struct remove_at<container<T, Ts...>, I>
    : return_type<push_front_t<remove_at_t<container<Ts...>, I - 1>, T>> {};

template <typename T, typename... Ts, Container container>
struct remove_at<container<T, Ts...>, 0> : return_type<container<Ts...>> {};

template <typename container, size_t I>
struct type_at;
template <typename container, size_t I>
using type_at_t = deref<type_at<container, I>>;
template <typename T, typename... Ts, Container container, size_t I>
struct type_at<container<T, Ts...>, I> : type_at<container<Ts...>, I - 1> {};
template <typename T, typename... Ts, Container container>
struct type_at<container<T, Ts...>, 0> : return_type<T> {};

template <Container container, size_t I>
struct type_at<container<>, I> {
    static_assert(false, "index out of range");
};

template <typename C, typename U, size_t start = 0, size_t end = size_of_v<C>>
struct find_first;
template <typename C, typename U, size_t start = 0, size_t end = size_of_v<C>>
static constexpr size_t find_first_v = find_first<C, U, start, end>::value;
template <typename C, typename U, size_t start, size_t end>
struct find_first : conditional_t<std::is_same_v<type_at_t<C, start>, U>,
                                  integral_constant<size_t, start>,
                                  find_first<C, U, start + 1, end>> {};
template <typename C, typename U, size_t end>
struct find_first<C, U, end, end> {
    static_assert(false, "index out of range");
};
template <typename c, typename From, typename To>
struct replace_one;
template <typename c, typename From, typename To>
using replace_one_t = deref<replace_one<c, From, To>>;
template <typename C, typename From, typename To>
struct replace_one
    : return_type<insert_at_t<remove_at_t<C, find_first_v<C, From>>,
                              find_first_v<C, From>, To>> {};

template <size_t I, typename... T>
struct bind_at;

template <typename T>
constexpr bool is_bind_at_arg_v = false;
template <typename T, size_t I>
constexpr bool is_bind_at_arg_v<bind_at<I, T>> = true;

template <typename C, typename B, size_t I = 0>
struct batch_insert;
template <typename C, typename B, size_t I = 0>
using batch_insert_t = deref<batch_insert<C, B, I>>;
template <typename C, size_t I, size_t n, typename... Arg, typename... T>
struct batch_insert<C, type_record<bind_at<I, Arg...>, T...>, n>
    : batch_insert<insert_at_t<C, n + I, Arg...>, type_record<T...>,
                   n + sizeof...(Arg)> {};
template <typename C, size_t n, typename Arg, typename... T>
struct batch_insert<C, type_record<Arg, T...>, n>
    : batch_insert<insert_at_t<C, n, Arg>, type_record<T...>, n + 1> {};
template <typename C, size_t n>
struct batch_insert<C, type_record<>, n> : return_type<C> {};

template <Container tmp, typename... T>
struct bind {
    using type = bind<tmp, T...>;
};
template <Container tmp, typename... T>
using bind_t = bind<tmp, T...>::type;

template <typename... T, typename... B, Container tmp>
struct invoke_meta<bind<tmp, B...>, T...>
    : repack_t<batch_insert_t<type_record<T...>, type_record<B...>>, tmp> {};

template <typename T>
using as_type_record_t = repack_t<T, type_record>;

template <typename T, typename... Ts>
struct concat;

template <typename T, typename... Ts>
using concat_t = deref<concat<T, Ts...>>;

template <Container container, typename... T, typename... Ts, typename... R>
struct concat<container<T...>, container<Ts...>, R...>
    : return_type<concat_t<container<T..., Ts...>, R...>> {};
template <Container container, typename... T, typename... Ts>
struct concat<container<T...>, container<Ts...>>
    : return_type<container<T..., Ts...>> {};
template <Container container>
struct concat<container<>, container<>> : return_type<container<>> {};

template <typename T, size_t max_recusive = 16>
struct flatten;
template <typename T, size_t max_recusive = 16>
using flatten_t = deref<flatten<T, max_recusive>>;
template <typename... Ts, Container container, size_t max_recusive>
struct flatten<container<Ts...>, max_recusive> : return_type<container<Ts...>> {
};
template <typename... T, typename... Ts, Container container,
          size_t max_recusive>
struct flatten<container<container<T...>, Ts...>, max_recusive>
    : return_type<concat_t<flatten_t<container<T...>, max_recusive - 1>,
                           flatten_t<container<Ts...>, max_recusive>>> {};
template <typename... T, typename... Ts, Container container>
struct flatten<container<container<T...>, Ts...>, 0>
    : return_type<container<container<T...>, Ts...>> {};
template <typename T, typename... Ts, Container container, size_t max_recusive>
struct flatten<container<T, Ts...>, max_recusive>
    : return_type<push_front_t<flatten_t<container<Ts...>, max_recusive>, T>> {
};
template <typename T, typename... Ts, Container container>
struct flatten<container<T, Ts...>, 0> : return_type<container<T, Ts...>> {};

template <typename T, typename predicate>
struct remove_if;
template <typename T, typename predicate>
using remove_if_t = deref<remove_if<T, predicate>>;
template <typename predicate, Container container, typename... Ts>
struct remove_if<container<Ts...>, predicate>
    : concat<remove_if_t<container<Ts>, predicate>...> {};
template <typename predicate, Container container, typename T>
struct remove_if<container<T>, predicate>
    : return_type<conditional_t<invoke_meta_v<predicate, T>, container<>,
                                container<T>>> {};
template <typename predicate, Container container>
struct remove_if<container<>, predicate> : return_type<container<>> {};

template <typename C, typename predicate>
struct filter_if;
template <typename C, typename predicate>
using filter_if_t = deref<filter_if<C, predicate>>;
template <typename C, typename predicate>
struct filter_if : remove_if<C, negation<predicate>> {};

template <typename from, typename applies>
struct batch_transform;
template <typename from, typename applies>
using batch_transform_t = deref<batch_transform<from, applies>>;

template <typename... T, template <typename...> class... fn>
struct batch_transform<type_record<T...>, template_record<fn...>>
    : return_type<type_record<fn<T...>...>> {};

template <typename T, typename fn>
struct transform;
template <typename T, typename fn>
using transform_t = deref<transform<T, fn>>;
template <Container C, typename... T, typename fn>
struct transform<C<T...>, fn> : return_type<C<invoke_meta_t<fn, T>...>> {};

template <typename T, typename predicate, typename fn>
struct transform_if;
template <typename T, typename predicate, typename fn>
using transform_if_t = deref<transform_if<T, predicate, fn>>;
template <Container C, typename predicate, typename... T, typename fn>
struct transform_if<C<T...>, predicate, fn>
    : return_type<C<conditional_t<invoke_meta_v<predicate, T>,
                                  invoke_meta_t<fn, T>, T>...>> {};
template <typename T>
struct transfer_to;
template <typename T, typename... U>
struct invoke_meta<transfer_to<T>, U...> : return_type<T> {};

template <typename C, typename Init, typename fn>
struct fold_left;
template <typename C, typename Init, typename fn>
using fold_left_t = deref<fold_left<C, Init, fn>>;
template <typename T, typename... Ts, typename Init, Container C, typename fn>
struct fold_left<C<T, Ts...>, Init, fn>
    : return_type<fold_left_t<C<Ts...>, invoke_meta_t<fn, Init, T>, fn>> {};
template <typename Init, Container C, typename fn>
struct fold_left<C<>, Init, fn> : return_type<Init> {};

template <typename C, size_t i, typename T>
struct replace_at;
template <typename C, size_t i, typename T>
using replace_at_t = deref<replace_at<C, i, T>>;
template <typename C, size_t i, typename T>
struct replace_at : return_type<insert_at_t<remove_at_t<C, i>, i, T>> {};

template <typename C, size_t i, size_t j>
struct swap;
template <typename C, size_t i, size_t j>
using swap_t = deref<swap<C, i, j>>;
template <typename C, size_t i, size_t j>
struct swap
    : replace_at<replace_at_t<C, i, type_at_t<C, j>>, j, type_at_t<C, i>> {};

template <typename T, typename U>
struct less;
template <typename T, typename U, T t, U u>
struct less<std::integral_constant<T, t>, std::integral_constant<U, u>>
    : std::integral_constant<bool, (t < u)> {};
template <typename T, typename U>
struct greater;
template <typename T, typename U, T t, U u>
struct greater<std::integral_constant<T, t>, std::integral_constant<U, u>>
    : std::integral_constant<bool, (t > u)> {};

using less_then = bind<less>;
using greater_then = bind<greater>;

template <typename T, typename... U>
struct is_same_specialized : false_type {};
template <typename T, typename... U>
constexpr bool is_same_specialized_v = is_same_specialized<T, U...>::value;
template <Container C, typename... T, typename... U>
struct is_same_specialized<C<T...>, U...>
    : std::conjunction<is_specialized<C, U>...> {};

template <auto... v>
struct value_record;
template <auto... v>
constexpr size_t size_of_v<value_record<v...>> = sizeof...(v);
template <typename C>
struct as_integral_type_record;
template <typename C>
using as_integral_type_record_t = deref<as_integral_type_record<C>>;
template <template <auto...> typename C, auto... v>
struct as_integral_type_record<C<v...>>
    : return_type<type_record<std::integral_constant<decltype(v), v>...>> {};
template <typename C>
struct as_value_record;
template <typename C>
using as_value_record_t = deref<as_value_record<C>>;
template <Container C, typename... T, T... v>
struct as_value_record<C<std::integral_constant<T, v>...>>
    : return_type<value_record<v...>> {};

template <bool t, auto v>
constexpr std::enable_if_t<t, decltype(v)> enable_if_v = v;

template <typename C>
struct empty_or_pop_front : return_type<C> {};
template <typename C>
using empty_or_pop_front_t = deref<empty_or_pop_front<C>>;
template <typename T, typename... Ts, Container C>
struct empty_or_pop_front<C<T, Ts...>> : return_type<C<Ts...>> {};

template <typename C1, typename C2, typename comp = less_then,
          size_t offset = 0, size_t size = size_of_v<C1>, typename = void>
struct order_preserving_merge;
template <typename C1, typename C2, typename comp = less_then,
          size_t offset = 0>
using order_preserving_merge_t =
    deref<order_preserving_merge<C1, C2, comp, offset>>;
template <typename C1, Container C2, typename comp, typename N, typename... Ns,
          size_t offset, size_t size>
struct order_preserving_merge<C1, C2<N, Ns...>, comp, offset, size,
                              std::enable_if_t<(size > offset)>>
    : return_type<conditional_t<
          (size_of_v<C1> == offset), concat_t<C1, C2<N, Ns...>>,
          conditional_t<
              invoke_meta_v<comp, type_at_t<C1, offset>, N>,
              order_preserving_merge_t<C1, C2<N, Ns...>, comp, offset + 1>,
              order_preserving_merge_t<insert_at_t<C1, offset, N>, C2<Ns...>,
                                       comp, offset>>>> {};
template <typename C1, Container C2, typename comp, size_t offset, size_t size>
struct order_preserving_merge<C1, C2<>, comp, offset, size> : return_type<C1> {
};
template <typename C2, Container C1, size_t size, typename comp>
struct order_preserving_merge<C1<>, C2, comp, 0, size> : return_type<C2> {};
template <typename C1, typename C2, typename comp, size_t offset>
struct order_preserving_merge<C1, C2, comp, offset, offset>
    : return_type<concat_t<C1, C2>> {};

template <typename C, size_t offset, size_t size = size_of_v<C> - offset,
          typename = void>
struct slice_of;
template <typename C, size_t offset, size_t size = size_of_v<C> - offset>
using slice_of_t = deref<slice_of<C, offset, size>>;
template <Container C, typename T, typename... Ts, size_t offset, size_t size>
struct slice_of<C<T, Ts...>, offset, size,
                std::enable_if_t<(size != 0 && offset <= sizeof...(Ts))>>
    : return_type<conditional_t<
          offset == 0, push_front_t<slice_of_t<C<Ts...>, 0, size - 1>, T>,
          slice_of_t<C<Ts...>, offset - 1, size>>> {};
template <Container C, typename... Ts, size_t offset, size_t size>
struct slice_of<C<Ts...>, offset, size,
                std::enable_if_t<!(size != 0 && offset <= sizeof...(Ts))>>
    : return_type<C<>> {};

template <typename T>
struct is_type_container : false_type {};
template <typename T>
constexpr bool is_type_container_v = is_type_container<T>::value;
template <Container C, typename... Ts>
struct is_type_container<C<Ts...>> : true_type {};
template <Container C>
struct is_type_container<C<>> : true_type {};

template <typename C, typename comp = less_then, typename = void>
struct sort;
template <typename C, typename comp = less_then>
using sort_t = deref<sort<C, comp>>;
template <typename C, typename comp>
struct sort<C, comp,
            std::enable_if_t<(2 > size_of_v<C> && is_type_container_v<C>)>>
    : return_type<C> {};
template <typename C, typename comp>
struct sort<C, comp,
            std::enable_if_t<(2 == size_of_v<C> && is_type_container_v<C>)>>
    : return_type<
          conditional_t<(invoke_meta_v<comp, type_at_t<C, 0>, type_at_t<C, 1>>),
                        C, swap_t<C, 0, 1>>> {};
template <typename C, typename comp>
struct sort<C, comp,
            std::enable_if_t<(size_of_v<C> > 2 && is_type_container_v<C>)>>
    : order_preserving_merge<
          sort_t<slice_of_t<C, 0, size_of_v<C> / 2>, comp>,
          sort_t<slice_of_t<C, size_of_v<C> / 2,
                            (size_of_v<C> - size_of_v<C> / 2)>,
                 comp>,
          comp> {};

template <auto... v, typename comp>
struct sort<value_record<v...>, comp, void>
    : as_value_record<
          sort_t<as_integral_type_record_t<value_record<v...>>, comp>> {};

}  // namespace xc::traits

#undef Container
