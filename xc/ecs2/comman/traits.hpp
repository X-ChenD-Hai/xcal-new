#pragma once
#include <cstddef>
#include <type_traits>

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
using invoke_mata_t = deref<invoke_meta<fn, T...>>;

template <template <typename...> typename Tmp, typename T>
struct is_specialized : false_type {};
template <template <typename...> typename Tmp, typename T>
constexpr bool is_specialized_v = is_specialized<Tmp, T>::value;
template <typename T, template <typename...> typename Tmp>
constexpr bool specialized_from_v = is_specialized<Tmp, T>::value;
template <template <typename...> typename Tmp, typename... T>
struct is_specialized<Tmp, Tmp<T...>> : true_type {};
template <template <typename...> typename Tmp>
struct specialized_from;
template <typename... T, template <typename...> typename Tmp>
struct invoke_meta<specialized_from<Tmp>, T...> : return_type<Tmp<T...>> {};

template <template <typename...> typename Tmp>
struct is_specialized_from;
template <typename... T, template <typename...> typename Tmp>
struct invoke_meta<is_specialized_from<Tmp>, Tmp<T...>> : true_type {};
template <typename T, template <typename...> typename Tmp>
struct invoke_meta<is_specialized_from<Tmp>, T> : false_type {};

template <template <typename...> typename... T>
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

template <template <typename...> typename... Tmp>
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

template <template <typename...> typename container, typename... T,
          typename... Ts>
struct push_front<container<Ts...>, T...>
    : return_type<container<T..., Ts...>> {};
template <typename T, template <typename...> typename container>
struct repack;
template <typename T, template <typename...> typename container>
using repack_t = deref<repack<T, container>>;
template <typename... T, template <typename...> typename o,
          template <typename...> typename container>
struct repack<o<T...>, container> : public return_type<container<T...>> {};
template <typename T,
          template <template <typename...> typename...> typename container>
struct repack_template;
template <typename T,
          template <template <typename...> typename...> typename container>
using repack_template_t = deref<repack_template<T, container>>;
template <template <typename...> typename... T,
          template <template <typename...> typename...> typename o,
          template <template <typename...> typename...> typename container>
struct repack_template<o<T...>, container>
    : public return_type<container<T...>> {};

template <typename container, typename predicate>
struct count_if;
template <typename container, typename predicate>
static constexpr size_t count_if_v = count_if<container, predicate>::value;
template <typename... T, template <typename...> typename container,
          typename predicate>
struct count_if<container<T...>, predicate>
    : integral_constant<size_t, (invoke_meta_v<predicate, T> + ...)> {};
template <template <typename...> typename container, typename predicate>
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
template <template <typename...> typename container, typename... T>
struct size_of<container<T...>> : integral_constant<size_t, sizeof...(T)> {};

template <typename container, size_t I, typename... T>
struct insert_at;
template <typename container, size_t I, typename... T>
using insert_at_t = deref<insert_at<container, I, T...>>;
template <template <typename...> typename container, typename... U>
struct insert_at<container<>, 0, U...> : return_type<container<U...>> {};
template <typename T, typename... Ts, template <typename...> typename container,
          size_t I, typename... U>
struct insert_at<container<T, Ts...>, I, U...>
    : return_type<
          insert_at_t<insert_at_t<container<Ts...>, I - 1, U...>, 0, T>> {};
template <typename T, typename... Ts, template <typename...> typename container,
          typename... U>
struct insert_at<container<T, Ts...>, 0, U...>
    : return_type<container<U..., T, Ts...>> {};

template <typename container, size_t I>
struct remove_at;
template <typename container, size_t I>
using remove_at_t = deref<remove_at<container, I>>;
template <typename T, typename... Ts, template <typename...> typename container,
          size_t I>
struct remove_at<container<T, Ts...>, I>
    : return_type<push_front_t<remove_at_t<container<Ts...>, I - 1>, T>> {};

template <typename T, typename... Ts, template <typename...> typename container>
struct remove_at<container<T, Ts...>, 0> : return_type<container<Ts...>> {};

template <typename container, size_t I>
struct type_at;
template <typename container, size_t I>
using type_at_t = deref<type_at<container, I>>;
template <typename T, typename... Ts, template <typename...> typename container,
          size_t I>
    requires(I <= sizeof...(Ts))
struct type_at<container<T, Ts...>, I> : type_at<container<Ts...>, I - 1> {};
template <typename T, typename... Ts, template <typename...> typename container>
struct type_at<container<T, Ts...>, 0> : return_type<T> {};

template <template <typename...> typename container, size_t I>
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

template <template <typename...> typename tmp, typename... T>
struct bind {
    using type = bind<tmp, T...>;
};
template <template <typename...> typename tmp, typename... T>
using bind_t = bind<tmp, T...>::type;

template <typename... T, typename... B, template <typename...> typename tmp>
struct invoke_meta<bind<tmp, B...>, T...>
    : repack_t<batch_insert_t<type_record<T...>, type_record<B...>>, tmp> {};

template <typename T>
using as_type_record_t = repack_t<T, type_record>;

template <typename T, typename... Ts>
struct concat;

template <typename T, typename... Ts>
using concat_t = deref<concat<T, Ts...>>;

template <template <typename...> typename container, typename... T,
          typename... Ts, typename... R>
struct concat<container<T...>, container<Ts...>, R...>
    : return_type<concat_t<container<T..., Ts...>, R...>> {};
template <template <typename...> typename container, typename... T,
          typename... Ts>
struct concat<container<T...>, container<Ts...>>
    : return_type<container<T..., Ts...>> {};

template <typename T, size_t max_recusive = 16>
struct flatten;
template <typename T, size_t max_recusive = 16>
using flatten_t = deref<flatten<T, max_recusive>>;
template <typename... Ts, template <typename...> typename container,
          size_t max_recusive>
struct flatten<container<Ts...>, max_recusive> : return_type<container<Ts...>> {
};
template <typename... T, typename... Ts,
          template <typename...> typename container, size_t max_recusive>
struct flatten<container<container<T...>, Ts...>, max_recusive>
    : return_type<concat_t<flatten_t<container<T...>, max_recusive - 1>,
                           flatten_t<container<Ts...>, max_recusive>>> {};
template <typename... T, typename... Ts,
          template <typename...> typename container>
struct flatten<container<container<T...>, Ts...>, 0>
    : return_type<container<container<T...>, Ts...>> {};
template <typename T, typename... Ts, template <typename...> typename container,
          size_t max_recusive>
struct flatten<container<T, Ts...>, max_recusive>
    : return_type<push_front_t<flatten_t<container<Ts...>, max_recusive>, T>> {
};
template <typename T, typename... Ts, template <typename...> typename container>
struct flatten<container<T, Ts...>, 0> : return_type<container<T, Ts...>> {};

template <typename T, typename predicate>
struct remove_if;
template <typename T, typename predicate>
using remove_if_t = deref<remove_if<T, predicate>>;
template <typename predicate, template <typename...> typename container,
          typename T, typename... Ts>
struct remove_if<container<T, Ts...>, predicate>
    : return_type<conditional_t<
          invoke_meta_v<predicate, T>, remove_if_t<container<Ts...>, predicate>,
          push_front_t<remove_if_t<container<Ts...>, predicate>, T>>> {};
template <typename predicate, template <typename...> typename container>
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
}  // namespace xc::traits