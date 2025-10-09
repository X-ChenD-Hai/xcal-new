#pragma once
#include <functional>
#include <print>
#include <reflectionrecord.hpp>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

enum class EditFieldType {
    String,
    Int,
    Float,
    Bool,
};

struct EditorField {
    std::string name;
    EditFieldType type;
    std::variant<std::string, int, float, bool> value;
};

template <typename T>
struct FieldConverter;
template <typename T>
    requires(std::is_constructible_v<T, std::string> ||
             std::is_same_v<T, std::string>)
struct FieldConverter<T> {
    using type = std::string;
    static constexpr auto type_code = EditFieldType::String;
};
template <typename T>
    requires(std::is_integral_v<T>)
struct FieldConverter<T> {
    using type = int;
    static constexpr auto type_code = EditFieldType::Int;
};
template <typename T>
    requires(std::is_floating_point_v<T>)
struct FieldConverter<T> {
    using type = float;
    static constexpr auto type_code = EditFieldType::Float;
};
template <typename T>
    requires(std::is_same_v<T, bool>)
struct FieldConverter<T> {
    using type = bool;
    static constexpr auto type_code = EditFieldType::Bool;
};

template <typename T>
struct FieldConverter
    : public FieldConverter<std::remove_reference_t<std::remove_cv_t<T>>> {};
template <size_t N>
struct FieldConverter<char[N]> : public FieldConverter<std::string> {};

template <typename T>
struct FunctionTraits;
template <typename R, typename... Args>
struct FunctionTraits<R(Args...)> {
    using return_type = R;
    using arg_types = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);
    static constexpr auto is_function_objec = false;
    static constexpr auto is_member_function = false;
};
template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> : public FunctionTraits<R(Args...)> {};
template <typename R, typename... Args>
struct FunctionTraits<R (&)(Args...)> : public FunctionTraits<R(Args...)> {};
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> : public FunctionTraits<R(Args...)> {
    static constexpr auto is_member_function = true;
};
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const>
    : public FunctionTraits<R(Args...)> {
    static constexpr auto is_member_function = true;
};
template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())> {
    static constexpr auto is_function_objec = true;
};

class Editor {
    std::string title_;
    std::vector<EditorField> fields_;
    std::function<void(Editor&)> callback_;

   protected:
    template <typename T>
    void set_field_(size_t index, T&& value, std::string_view name = "") {
        if (index >= fields_.size() ||
            fields_[index].type != FieldConverter<T>::type_code) {
            throw std::runtime_error("Invalid field index or type");
        }
        if (!name.empty()) fields_[index].name = name;
        fields_[index].value = std::forward<T>(value);
    }

   public:
    template <typename Fn, typename... NameArgs>
        requires(sizeof...(NameArgs) == FunctionTraits<Fn>::arity * 2)
    Editor(Fn&& fn, std::string title, NameArgs&&... args) : title_(title) {
        using Ft = FunctionTraits<Fn>;
        fields_.reserve(sizeof...(NameArgs) / 2);
        [&]<typename Arg1, typename Arg2, typename... Args>(
            this auto&& self, Arg1&& name, Arg2&& arg1, Args&&... args) {
            fields_.emplace_back(std::string(name),
                                 FieldConverter<Arg2>::type_code,
                                 std::forward<Arg2>(arg1));
            if constexpr (sizeof...(Args) > 0) {
                self(std::forward<Args>(args)...);
            }
        }(std::forward<NameArgs>(args)...);

        callback_ = [fn](Editor& eidtor) {
            [&]<size_t... I>(std::index_sequence<I...>) {
                if constexpr (Ft::is_member_function) {
                }
                fn([&]() {
                    using At = std::remove_reference_t<decltype(std::get<I>(
                        std::declval<typename Ft::arg_types>()))>;
                    try {
                        return std::get<At>(eidtor.fields_[I].value);
                    } catch (...) {
                        std::println("Invalid input");
                        return At{};
                    }
                }()...);
            }(std::make_index_sequence<(Ft::arity)>{});
        };
    }
    void update() { callback_(*this); }
    template <typename T>
    void set_field(size_t index, T&& value, std::string_view name = "") {
        set_field_(index, std::forward<T>(value), name);
        update();
    }
    template <typename... Args>
    void set_fields(Args&&... args) {
        int index = 0;
        (set_fields_(index++, std::forward<Args>(args)), ...);
        update();
    };
    const auto& fields() const { return fields_; }
    auto& fields() { return fields_; }
    const auto& title() const { return title_; }
    auto& title() { return title_; }
};