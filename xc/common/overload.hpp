#include <type_traits>

template <class T>
struct Overload;

template <class R, class... Args>
struct Overload<R(Args...)> {
    static constexpr decltype(auto) of(R(f)(Args...)) { return f; }
    template <class C>
        requires(!std::is_const_v<C>)
    static constexpr decltype(auto) of(R (C::*f)(Args...)) {
        return f;
    }
    template <class C>
        requires(std::is_const_v<C>)
    static constexpr decltype(auto) of(R (C::*f)(Args...) const) {
        return f;
    }
    template <class C>
    static constexpr decltype(auto) const_of(R (C::*f)(Args...) const) {
        return f;
    }
};