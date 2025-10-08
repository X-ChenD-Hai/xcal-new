#include <array>
#include <iostream>
#include <string_view>

template <class _Type>
consteval std::string_view type_string() {
#ifdef __clang__
    static constexpr std::string_view s{__PRETTY_FUNCTION__};
    static constexpr auto si = s.find("_Type = ") + sizeof("_Type = ") - 1;
    return s.substr(si, s.size() - si - 1);
#elif defined(__GNUC__)
    static constexpr std::string_view s{__PRETTY_FUNCTION__};
    static constexpr auto si = s.find("_Type = ") + sizeof("_Type = ") - 1;
    return s.substr(si, s.find("; std") - si);
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    static constexpr std::string_view s{__FUNCSIG__};
    static constexpr auto si = s.find(__FUNCTION__) + sizeof(__FUNCTION__) + 1;
    static constexpr auto ei = s.rfind(">(void)");
    static constexpr auto ns = s.substr(si - 1, ei - si + 1);
    if constexpr (ns.find("enum ") == 0)
        return ns.substr(sizeof("enum"), ns.size() - sizeof("enum"));
    else
        return ns;
#endif
    return "";
}

template <class _Enum, _Enum _Value>
    requires std::is_enum_v<_Enum>
consteval std::string_view enum_value_name() {
#if defined(__GNUC__)
    static constexpr std::string_view s{__PRETTY_FUNCTION__};
    static constexpr auto ei = s.rfind("; std::string_view = ");
    static constexpr auto si = s.substr(0, ei).rfind("::") + 2;
    static constexpr auto ns = s.substr(si, ei - si);
    if (ns.find(")") != std::string_view::npos)
        return ns.substr(ns.find(")") + 1, ns.size() - ns.find(")"));
    else
        return ns;
#elif defined(__clang__)
    static constexpr std::string_view s{__PRETTY_FUNCTION__};
    static constexpr auto si = s.rfind("::") + 2;
    static constexpr auto ei = s.rfind("]");
    static constexpr auto ns = s.substr(si, ei - si);
    if (ns.find(")") != std::string_view::npos)
        return ns.substr(ns.find(")") + 1, ns.size() - ns.find(")"));
    else
        return ns;
#elif defined(_MSC_VER)
    static constexpr std::string_view s{__FUNCSIG__};
    static constexpr auto si = s.rfind("::") + 2;
    static constexpr auto ei = s.rfind(">(void)");
    static constexpr auto ns = s.substr(si, ei - si);
    if constexpr (ns.find(")") != std::string_view::npos)
        return ns.substr(ns.find(")") + 1, ns.size() - ns.find(")") - 1);
    else
        return ns;
#endif
    return "";
}

template <class _Type, bool _IsEnum = std::is_enum_v<_Type>>
struct type_meta_info {
    static constexpr std::string_view name = type_string<_Type>();
    static constexpr size_t size = sizeof(_Type);
    static constexpr auto is_enum = std::is_enum_v<_Type>;
};
template <class _Enum>
struct type_meta_info<_Enum, true> : public type_meta_info<_Enum, false> {
    template <_Enum _Value>
    static constexpr auto value_name = enum_value_name<_Enum, _Value>();
};
template <auto _Value>
consteval auto value_name() {
    return enum_value_name<decltype(_Value), _Value>();
}
template <class _Enum, _Enum _Value, class _VTp>
struct allocator {};
template <class _Enum, class _VTp>
struct EnumMap {
    template <size_t _Start, size_t _End>
    static constexpr auto value_map{
        []<size_t... _Idx>(std::index_sequence<_Idx...>) {
            return std::array<_VTp (*)(), sizeof...(_Idx)>{
                allocator<_Enum, (_Enum)_Idx, _VTp>::allocate...};
        }(std::make_index_sequence<_End - _Start>{})};
};
template <auto _Value, class _VTp>
struct EnumMapItem {
    constexpr static _VTp allocate() {
        return EnumMap<decltype(_Value), _VTp>::template value_map<0, 128>[(
            size_t)_Value]();
    }
};

template <auto _Value>
    requires std::is_enum_v<std::remove_cvref_t<decltype(_Value)>>
struct StaticReflection {
    static_assert(false, "not implemented");
};
template <class _Type>
struct ReflectionRecord {
    using type = _Type;
};
template <class _Base>
struct InstenceReflection {
    // static constexpr std::unordered_map<void*> members;
};
template <class _Type, class _Base, auto _Value>
struct Record {
    using type = _Type;
};

namespace A {
enum class Color { Red, Green, Blue };
class AbsColor {};
class Red : public AbsColor {};
class Green : public AbsColor {};
class Blue : public AbsColor {};
}  // namespace A

template <>
struct StaticReflection<A::Color::Red> : public ReflectionRecord<A::Red> {};

template <auto _Value>
    requires std::is_enum_v<std::remove_cvref_t<decltype(_Value)>>
struct ReflectionGetType {
    using type = StaticReflection<_Value>::type;
};
template <A::Color _Value>
struct allocator<A::Color, _Value, A::AbsColor*> {
    static A::AbsColor* allocate() { return nullptr; }
};
template <>
struct allocator<A::Color, A::Color::Red, A::AbsColor*> {
    static A::AbsColor* allocate() { return new A::Red(); }
};
template <>
struct allocator<A::Color, A::Color::Green, A::AbsColor*> {
    static A::AbsColor* allocate() { return new A::Green(); }
};
template <>
struct allocator<A::Color, A::Color::Blue, A::AbsColor*> {
    static A::AbsColor* allocate() { return new A::Blue(); }
};

int main(int argc, char* argv[]) {
    std::cout << type_string<int>() << std::endl;
    std::cout << type_string<A::Color>() << std::endl;

    std::cout << value_name<A::Color::Red>() << std::endl;
    std::cout << value_name<A::Color::Green>() << std::endl;
    std::cout << value_name<A::Color::Blue>() << std::endl;
    std::cout << value_name<A::Color(11)>() << std::endl;

    using tt = ReflectionGetType<A::Color::Red>::type;

     A::Red r{};
    void* ins = EnumMapItem<A::Color::Red, A::AbsColor*>::allocate();

    // std::cout << type_string<tt>() << std::endl;

    return 0;
}