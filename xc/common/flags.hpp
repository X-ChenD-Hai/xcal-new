/**
 * @file enum.hpp
 * @author X_Chen D_Hai (illuminatestar@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-10-01
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <array>
#include <format>
#include <vector>
namespace flags {
template <class _Type, bool _IsEnum = std::is_enum_v<_Type>>
struct type_meta_info {
    static constexpr std::string_view name = []() {
        constexpr std::string_view s{__PRETTY_FUNCTION__};
        constexpr std::string_view first = "[_Type = ";
        constexpr std::string_view mid = ", _IsEnum = ";
        constexpr auto si = s.find(first) + first.size();
        constexpr auto mi = s.find(mid);
        return s.substr(si, mi - si);
    }();
    static constexpr size_t size = sizeof(_Type);
    static constexpr size_t is_enum = std::is_enum_v<_Type>;
};
template <class _Type>
struct type_meta_info<_Type, true> : public type_meta_info<_Type, false> {
    template <_Type _Val>
    struct value {
        static constexpr std::string_view name = []() {
            constexpr std::string_view s{__PRETTY_FUNCTION__};
            constexpr std::string_view mid = "_Val = ";
            constexpr auto mi = s.rfind(mid);
            constexpr auto el_ = s.rfind("::");
            if constexpr (el_ != std::string_view::npos && el_ > mi) {
                return s.substr(el_ + 2, s.size() - el_ - 3);
            } else {
                return "";
            }
        }();
    };
};

template <class A, A a>
    requires std::is_enum_v<A>
consteval std::string_view static_enum_value_name() {
    return type_meta_info<A>::template value<a>::name;
}
template <class A>
consteval std::string_view static_type_name() {
    return type_meta_info<A>::name;
}

template <class Enum>
concept EnumType = requires { requires(Enum::__MAX__ > 0); };

template <class Enum, size_t start = 0, size_t end = std::string_view::npos>
constexpr std::string_view enum_value_name(Enum a) {
    constexpr size_t e = []() {
        if constexpr (end == std::string_view::npos) {
            if constexpr (EnumType<Enum>) {
                return static_cast<size_t>(Enum::__MAX__);
            } else {
                return 128;
            }
        } else {
            return end;
        }
    }();
    static constexpr auto names = []<size_t... idx>(
                                      std::index_sequence<idx...>) {
        return std::array<std::string_view, sizeof...(idx)>{
            static_enum_value_name<Enum, static_cast<Enum>(start + idx)>()...};
    }(std::make_index_sequence<e - start>());
    if (static_cast<size_t>(a) + start >= names.size()) return "";
    return names[static_cast<size_t>(a) - start];
}
template <class Flag>
constexpr std::string_view flag_value_name(Flag flag) {
    constexpr auto names = []<size_t... idx>(std::index_sequence<idx...>) {
        return std::array<std::string_view, sizeof...(idx)>{
            static_enum_value_name<Flag, static_cast<Flag>(1 << idx)>()...};
    }(std::make_index_sequence<sizeof(Flag) * 8>());
    using data_t = std::underlying_type_t<Flag>;
    for (size_t i = 0; i < sizeof(Flag) * 8; ++i) {
        if (static_cast<data_t>(flag) & (1 << i)) {
            return names[i];
        }
    }
    return "";
}

template <class Flag>
std::vector<std::string_view> flag_value_list(
    std::underlying_type_t<Flag> flag) {
    std::vector<std::string_view> names;
    for (size_t i = 0; i < sizeof(Flag) * 8; ++i) {
        if (flag & (1 << i)) {
            names.push_back(flag_value_name<Flag>(static_cast<Flag>(1 << i)));
        }
    }
    return names;
}
template <class Flag>
std::string flag_value_list_str(std::underlying_type_t<Flag> flag) {
    std::vector<std::string_view> names = flag_value_list<Flag>(flag);
    if (names.empty()) return "";
    std::string s;
    s += names.front();
    names.erase(names.begin());
    for (auto name : names) {
        s += " | ";
        s += name;
    }
    return s;
}

template <class T>
    requires std::is_enum_v<T>
class Flags final {
    using data_t = std::underlying_type_t<T>;
    data_t data_;

   public:
    template <T flag>
    constexpr Flags() : data_(flag) {}
    constexpr Flags(data_t data = 0) : data_(data) {}
    constexpr Flags(T flag) : data_(static_cast<data_t>(flag)) {}
    template <class... Args>
        requires(std::is_convertible_v<Args, data_t> && ...)
    constexpr Flags(Args... args) : data_((static_cast<data_t>(args) | ...)) {}
    constexpr Flags &operator=(T flag) {
        data_ = static_cast<data_t>(flag);
        return *this;
    }
    constexpr Flags &operator|=(T flag) {
        data_ |= static_cast<data_t>(flag);
        return *this;
    }
    constexpr Flags &operator&=(T flag) {
        data_ &= static_cast<data_t>(flag);
        return *this;
    }
    constexpr Flags &operator^=(T flag) {
        data_ ^= static_cast<data_t>(flag);
        return *this;
    }
    constexpr Flags operator|(T flag) const {
        return Flags(data_ | static_cast<data_t>(flag));
    }
    constexpr Flags operator&(T flag) const {
        return Flags(data_ & static_cast<data_t>(flag));
    }
    constexpr Flags operator^(T flag) const {
        return Flags(data_ ^ static_cast<data_t>(flag));
    }
    bool has(T flag) const {
        return (data_ & static_cast<data_t>(flag)) == static_cast<data_t>(flag);
    }
    constexpr operator bool() const { return data_ != 0; }
    constexpr bool operator!() const { return data_ == 0; }
    std::string to_string() const { return flag_value_list_str<T>(data_); }

    constexpr explicit operator T() const { return static_cast<T>(data_); }
};

template <class T>
    requires std::is_enum_v<T>
std::ostream &operator<<(std::ostream &os, const Flags<T> &flags) {
    os << flags.to_string();
    return os;
}
template <class T>
    requires std::is_enum_v<T>
std::string to_string(const Flags<T> &flags) {
    return flags.to_string();
}

template <class T, class U>
concept FlagOp =
    requires { requires std::is_enum_v<T> && std::is_same_v<U, T>; };
template <class T, class U>
    requires FlagOp<T, U>
constexpr Flags<T> operator|(T flag1, U flag2) {
    return Flags<T>(flag1) | flag2;
}
template <class T, class U>
    requires FlagOp<T, U>
constexpr Flags<T> operator&(T flag1, U flag2) {
    return Flags<T>(flag1) & flag2;
}
template <class T, class U>
    requires FlagOp<T, U>
constexpr Flags<T> operator^(T flag1, U flag2) {
    return Flags<T>(flag1) ^ flag2;
}
}  // namespace flags

template <class T>
    requires std::is_enum_v<T>
struct std::formatter<flags::Flags<T>> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    auto format(const flags::Flags<T> &flags, format_context &ctx) const {
        return format_to(ctx.out(), "{}", flags.to_string());
    }
};
template <class T>
    requires std::is_enum_v<T>
struct std::formatter<T> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    auto format(T v, format_context &ctx) const {
        return format_to(ctx.out(), "{}", flags::enum_value_name<T>(v));
    }
};