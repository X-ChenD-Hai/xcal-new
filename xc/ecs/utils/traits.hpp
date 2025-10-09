#pragma once
#include <cstdint>
#include <functional>

template <typename... Ts>
struct tvector {
    template <typename T>
    static constexpr bool has = false || (std::is_same_v<T, Ts> || ...);
    static constexpr size_t size = sizeof...(Ts);
    template <size_t I>
    using at = std::tuple_element_t<I, std::tuple<Ts...>>;
    template <typename... T>
    using push_back = tvector<Ts..., T...>;
    template <typename... T>
    using push_front = tvector<T..., Ts...>;
    using pop_front =
        std::remove_pointer<decltype([]<size_t... I>(
                                         std::index_sequence<I...>) {
            return (tvector<at<I + 1>...> *)nullptr;
        }(std::make_index_sequence<size - 1>{}))>::type;
    using pop_back =
        std::remove_pointer<decltype([]<size_t... I>(
                                         std::index_sequence<I...>) {
            return (tvector<at<I>...> *)nullptr;
        }(std::make_index_sequence<size - 1>{}))>::type;
    template <typename T>
    using concat = std::remove_pointer<decltype([]<size_t... I>(
                                                    std::index_sequence<I...>) {
        return (tvector<Ts..., typename T::template at<I>...> *)nullptr;
    }(std::make_index_sequence<T::size>{}))>::type;

   private:
    template <typename... T>
    struct remove_all_helper {
        using Type = std::remove_pointer<decltype([]<class _T, class... _Ts>() {
            if constexpr (sizeof...(_Ts) == 0) {
                if constexpr (tvector<T...>::template has<_T>) {
                    return (tvector<> *)nullptr;
                } else {
                    return (tvector<_T> *)nullptr;
                }
            } else {
                if constexpr (tvector<T...>::template has<_T>) {
                    return (typename tvector<_Ts...>::template remove_all<T...>
                                *)nullptr;
                } else {
                    return (typename tvector<_Ts...>::template remove_all<
                            T...>::template push_front<_T> *)nullptr;
                }
            }
        }.template operator()<Ts...>())>::type;
    };
    template <>
    struct remove_all_helper<> {
        using Type = tvector<Ts...>;
    };

   public:
    template <size_t T>
        requires(T < size)
    using remove = std::remove_pointer<
        decltype([]<size_t... I, size_t... J>(std::index_sequence<I...>,
                                              std::index_sequence<J...>) {
            return (tvector<at<I>..., at<J + T + 1>...> *)nullptr;
        }(std::make_index_sequence<T>{},
                 std::make_index_sequence<size - T - 1>{}))>::type;
    template <typename... T>
    using remove_all = typename remove_all_helper<T...>::Type;
    template <typename T>
    using remove_all_from_list =
        std::remove_pointer<decltype([]<class... _Rt>(tvector<_Rt...> *) {
            return (remove_all<_Rt...> *)nullptr;
        }((T *)nullptr))>::type;
    template <class T>
    static constexpr size_t find =
        []<size_t I, class _T, class... _Ts>(this auto &&self) {
            if constexpr (std::is_same_v<T, _T>) {
                return I;
            }
            if constexpr (sizeof...(_Ts) == 0) {
                return I + 1;
            } else {
                return self.template operator()<I + 1, _Ts...>();
            }
        }.template operator()<0, Ts...>();
    template <size_t... T>
    using subsequence = tvector<at<T>...>;
    template <size_t Start, size_t End>
    using slice = std::remove_pointer<decltype([]<size_t... I>(
                                                   std::index_sequence<I...>) {
        return (tvector<at<Start + I>...> *)nullptr;
    }(std::make_index_sequence<End - Start>{}))>::type;
    template <typename Arg, typename... Args>
    struct remove_all_from_lists_helper {
        using type = remove_all_from_list<
            Arg>::template remove_all_from_lists<Args...>;
    };
    template <typename Arg>
    struct remove_all_from_lists_helper<Arg> {
        using type = remove_all_from_list<Arg>;
    };
    template <typename... T>
    using remove_all_from_lists =
        typename remove_all_from_lists_helper<T...>::type;
};
template <>
struct tvector<> {
    static constexpr size_t size = 0;
    template <typename T>
    static constexpr bool has = false;
    template <typename... T>
    using push_back = tvector<T...>;
    template <typename... T>
    using push_front = tvector<T...>;
    template <typename T>
    using concat = T;
    template <size_t... T>
        requires(sizeof...(T) == 0)
    using remove = tvector<>;
    template <size_t... T>
        requires(sizeof...(T) == 0)
    using remove_all = tvector<>;
    template <typename T>
    using remove_all_from_list = tvector<>;
    template <typename... T>
    using remove_all_from_lists = tvector<>;
};

namespace internal {
template <typename T>
struct Purge {
    using type = T;
};
template <typename T>
struct Purge<const T> {
    using type = Purge<T>::type;
};
template <typename T>
struct Purge<volatile T> {
    using type = Purge<T>::type;
};
template <typename T>
struct Purge<const volatile T> {
    using type = Purge<T>::type;
};
template <typename T>
struct Purge<T &> {
    using type = Purge<T>::type;
};
template <typename T>
struct Purge<T &&> {
    using type = Purge<T>::type;
};
template <typename T>
struct Purge<T *> {
    using type = Purge<T>::type;
};

template <typename T>
struct return_type_of;
template <typename T, typename... Args>
struct return_type_of<T(Args...)> {
    using type = T;
};
template <typename T, typename... Args>
struct return_type_of<T (*)(Args...)> {
    using type = T;
};
template <typename T>
struct first_arg_type_of;
template <typename T, typename Arg, typename... Args>
struct first_arg_type_of<T(Arg, Args...)> {
    using type = Purge<Arg>::type;
};
template <typename T>
struct first_arg_type_of<T(void)> {
    using type = void;
};
template <typename... T>
struct type_list {
    static constexpr size_t size = sizeof...(T);
};

template <typename T>
struct func_traits;
template <typename R, typename... Args>
struct func_traits<R(Args...)> {
    using return_type = R;
    using args = std::tuple<Args...>;
    using args_vec = tvector<Args...>;
    using self = R(*)(Args...);
    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool is_member_function = false;
};
template <typename R, typename... Args>
struct func_traits<R (*)(Args...)> : public func_traits<R(Args...)> {};
template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...)> : public func_traits<R(Args...)> {
    using Class = C;
    static constexpr bool is_member_function = true;
};
template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...) const>
    : public func_traits<R (C::*)(Args...)> {};

}  // namespace internal
template <auto Fn>
using return_type_of_t = internal::return_type_of<decltype(Fn)>::type;
template <auto Fn>
using first_arg_type_of_t = internal::first_arg_type_of<decltype(Fn)>::type;
// template <auto Fn , typename Args>

template <auto Fm>
using func_traits = internal::func_traits<decltype(Fm)>;
template <typename T>
using purge_t = typename internal::Purge<T>::type;
using component_t = uint32_t;
using archtype_t = uint32_t;
template <auto Fn, typename... Args>
constexpr auto is_func_with_args =
    std::is_constructible_v<std::function<return_type_of_t<Fn>(Args &...)>,
                            decltype(Fn)>;
