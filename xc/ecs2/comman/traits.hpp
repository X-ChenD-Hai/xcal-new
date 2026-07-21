#include <type_traits>

namespace details {
template <typename T>
struct return_type {
    using type = T;
};

template <typename T>
using deref = typename T::type;
template <template <typename...> typename Tmp, typename T>
struct is_specialized : std::false_type {};
template <template <typename...> typename Tmp, typename T>
constexpr bool is_specialized_v = is_specialized<Tmp, T>::value;
template <template <typename...> typename Tmp, typename... T>
struct is_specialized<Tmp, Tmp<T...>> : std::true_type {};

template <typename... T>
struct type_record;
template <template <typename...> typename... T>
struct template_record;

template <typename T, template <typename...> typename container>
struct repack;
template <typename T, template <typename...> typename container>
using repack_t = deref<repack<T, container>>;
template <typename... T, template <typename...> typename o,
          template <typename...> typename container>
struct repack<o<T...>, container> : public return_type<container<T...>> {};

template <typename T>
using as_type_record_t = repack_t<T, type_record>;

template <typename T, typename... Ts>
struct push_front;

template <typename T, typename... Ts>
using push_front_t = deref<push_front<T, Ts...>>;

template <template <typename...> typename container, typename... T,
          typename... Ts>
struct push_front<container<Ts...>, T...>
    : return_type<container<T..., Ts...>> {};

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

template <typename T>
struct flatten;
template <typename T>
using flatten_t = deref<flatten<T>>;
template <typename... Ts>
struct flatten<type_record<Ts...>> : return_type<type_record<Ts...>> {};
template <typename... T, typename... Ts>
struct flatten<type_record<type_record<T...>, Ts...>>
    : return_type<concat_t<flatten_t<type_record<T...>>,
                           flatten_t<type_record<Ts...>>>> {};
template <typename T, typename... Ts>
struct flatten<type_record<T, Ts...>>
    : return_type<push_front_t<flatten_t<type_record<Ts...>>, T>> {};

template <typename T, template <typename> typename predicate>
struct remove_if;

template <typename T, template <typename> typename predicate>
using remove_if_t = deref<remove_if<T, predicate>>;

template <template <typename T> typename predicate,
          template <typename...> typename container, typename T, typename... Ts>
struct remove_if<container<T, Ts...>, predicate>
    : return_type<std::conditional_t<
          predicate<T>::value, remove_if_t<container<Ts...>, predicate>,
          push_front_t<remove_if_t<container<Ts...>, predicate>, T>>> {};

template <template <typename T> typename predicate,
          template <typename...> typename container>
struct remove_if<container<>, predicate> : return_type<container<>> {};

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... T>
struct collect_marker;

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... T>
using collect_marker_t =
    deref<collect_marker<inc_unwrapper, marker, exclude_record, T...>>;

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record>
struct collect_marker<inc_unwrapper, marker, exclude_record>
    : return_type<marker<>> {};

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... T>
struct collect_marker<inc_unwrapper, marker, exclude_record, type_record<T...>>
    : return_type<
          collect_marker_t<inc_unwrapper, marker, exclude_record, T...>> {};

template <bool inc_unwrapper, template <typename...> typename marker,
          typename exclude_record, typename... Ts, typename... T>
struct collect_marker<inc_unwrapper, marker, exclude_record, marker<Ts...>,
                      T...>
    : return_type<std::conditional_t<
          (sizeof...(T) > 0),
          push_front_t<
              collect_marker_t<inc_unwrapper, marker, exclude_record, T...>,
              Ts...>,
          marker<Ts...>>> {};

template <template <typename...> typename marker,
          template <typename...> typename... ex_markers, typename T,
          typename... Ts>
    requires(!is_specialized_v<marker, T> && !is_specialized_v<type_record, T>)
struct collect_marker<true, marker, template_record<ex_markers...>, T, Ts...>
    : return_type<std::conditional_t<
          (sizeof...(Ts) > 0),
          std::conditional_t<
              (is_specialized_v<ex_markers, T> || ...) && sizeof...(ex_markers),
              collect_marker_t<true, marker, template_record<ex_markers...>,
                               Ts...>,
              push_front_t<
                  collect_marker_t<true, marker, template_record<ex_markers...>,
                                   Ts...>,
                  T>>,
          std::conditional_t<(is_specialized_v<ex_markers, T> || ...) &&
                                 sizeof...(ex_markers),
                             marker<>, marker<T>>>> {};

template <template <typename...> typename marker, typename exclude_record,
          typename T, typename... Ts>
    requires(!is_specialized_v<marker, T> && !is_specialized_v<type_record, T>)
struct collect_marker<false, marker, exclude_record, T, Ts...>
    : return_type<std::conditional_t<
          (sizeof...(Ts) > 0),
          collect_marker_t<false, marker, exclude_record, Ts...>, marker<>>> {};

}  // namespace details