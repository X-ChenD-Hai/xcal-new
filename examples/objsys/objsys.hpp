#pragma once
#include <cassert>
#include <functional>
#include <print>
#include <reflectionrecord.hpp>
#include <unordered_map>

class XC_OBJSYS_API Object{};

template <class _K, class _O>
struct mapper {
    using key_type = _K;
    using object_type = _O;
    bool set(_K k, _O o);
    void unset(_K k);
    _O get(_K k);
    template <class _Type>
    _K create_key();
    template <class _Type>
    _O create_value(_Type &obj);
    void names();
    void contains(key_type);
};
using ObkectCallback = std::function<void(Object &)>;
struct XC_OBJSYS_API NameMapper
    : public mapper<std::string_view, ObkectCallback> {
    std::unordered_map<std::string_view, ObkectCallback> map_{};
    bool set(std::string_view k, ObkectCallback &&func) {
        if (map_.contains(k)) {
            assert(false && "duplicate registration");
            return false;
        }
        map_[k] = std::move(func);
        return true;
    }
    bool unset(std::string_view k) { return map_.erase(k) > 0; }
    ObkectCallback get(std::string_view k) { return map_[k]; }
    bool contains(std::string_view k) const {
        return !map_.empty() && map_.contains(k);
    }
    std::vector<std::string_view> keys() const {
        std::vector<std::string_view> keys;
        for (auto &p : map_) {
            keys.push_back(p.first);
        }
        return keys;
    }
    auto names() const { return keys(); }
    template <class _Type>
    static consteval std::string_view create_key() {
        return type_string<_Type>();
    }
    template <class _V>
    ObkectCallback create_value(std::function<void(_V &)> f) {
        return [f = std::move(f)](Object &o) { f(static_cast<_V &>(o)); };
    }
};

template <class _Enum, size_t _Start, size_t _End>
    requires std::is_enum_v<_Enum>
struct EnumMappper : public mapper<_Enum, ObkectCallback> {
   private:
    using _Vtp = std::underlying_type_t<_Enum>;

   public:
    using key_type = _Enum;
    using object_type = ObkectCallback;
    std::array<ObkectCallback, _End - _Start> map_{nullptr};
    bool set(_Enum k, ObkectCallback &&func) {
        if ((_Vtp)k < _Start || (_Vtp)k >= _End) {
            assert(false && "out of range");
            return false;
        }
        if (map_[(_Vtp)k - _Start]) {
            assert(false && "duplicate registration");
            return false;
        }
        map_[(_Vtp)k - _Start] = std::move(func);
        return true;
    }
    bool unset(_Enum k) {
        if ((_Vtp)k < _Start || (_Vtp)k >= _End) {
            assert(false && "out of range");
            return false;
        }
        return map_[(_Vtp)k - _Start] = nullptr;
    }
    ObkectCallback get(_Enum k) {
        if ((_Vtp)k < _Start || (_Vtp)k >= _End) {
            assert(false && "out of range");
            return nullptr;
        }
        return map_[(_Vtp)k - _Start];
    }
    bool contains(key_type k) const {
        return (_Vtp)k >= _Start && (_Vtp)k < _End && map_[(_Vtp)k - _Start];
    }
    std::vector<key_type> keys() const {
        std::vector<key_type> keys;
        for (size_t i = 0; i < map_.size(); ++i) {
            if (map_[i]) {
                keys.push_back((_Enum)(i + _Start));
            }
        }
        return keys;
    }
    static consteval auto names() {
        return []<size_t... I>(std::index_sequence<I...>) {
            return std::array<std::string_view, sizeof...(I)>{
                enum_value_name<_Enum, _Enum(_Start + I)>()...};
        }(std::make_index_sequence<_End - _Start>{});
    }
    template <class _Type>
    static consteval auto create_key() {
        return _Type::Type;
    }
    template <class _V>
    ObkectCallback create_value(std::function<void(_V &)> f) {
        return [f = std::move(f)](Object &o) { f(static_cast<_V &>(o)); };
    }
};

template <class _M, class _K = _M::key_type, class _O = _M::object_type>
    requires std::derived_from<_M, mapper<_K, _O>>
class ObjectSystem {
   protected:
    _M map_;

   public:
    template <class _Type, class _Ot>
    bool register_class(_Ot o) {
        return map_.set(map_.template create_key<_Type>(),
                        map_.template create_value<_Type>(o));
    }
    bool register_class(_K k, _O o) { return map_.set(k, std::move(o)); }
    bool call_object(_K name, Object &obj) {
        if constexpr (!std::is_same_v<
                          decltype(std::declval<_M>().contains(
                              std::declval<typename _M::key_type>())),
                          void>) {
            if (map_.contains(name)) {
                map_.get(name)(obj);
                return true;
            }
            return false;
        } else {
            std::println("ObjectSystem::call_object: unsaved object");
            map_.get(name)(obj);
            return true;
        }
    }
    template <class _Type>
    bool call_object(Object &obj) {
        return call_object(map_.template create_key<_Type>(), obj);
    }

    auto class_keys() const { return map_.keys(); }
    auto class_names() const
        requires(!std::is_same_v<decltype(std::declval<_M>().names()), void>)
    {
        return map_.names();
    }
};

template <class Class>
class SignalInstance {
   public:
    static Class &instence() {
        static Class instance;
        return instance;
    }

   protected:
    SignalInstance() = default;
    ~SignalInstance() = default;
    SignalInstance(const SignalInstance &) = delete;
    SignalInstance(SignalInstance &&) = delete;
    SignalInstance &operator=(const SignalInstance &) = delete;
    SignalInstance &operator=(SignalInstance &&) = delete;
};
using StringObjectMap = SignalInstance<ObjectSystem<NameMapper>>;
template <class _Enum, size_t _Start, size_t _End>
using EnumObjectMap =
    SignalInstance<ObjectSystem<EnumMappper<_Enum, _Start, _End>>>;

template <class _T, class ObjectSystem = StringObjectMap>
class Base : public Object {
   private:
    inline _T *self() { return static_cast<_T *>(this); }
    inline const _T *self() const { return static_cast<_T *>(this); }
    static const bool reg;

   public:
    using BaseClass = Base<_T, ObjectSystem>;
    Base() { std::println("Base object created!"); }
    void call() { ObjectSystem::instence().template call_object<_T>(*self()); }

   protected:
    ~Base() { std::println("Base object destroyed!"); }
};
template <class _T, auto _Value, class ObjectSystem>
class EnumBase : public Base<_T, ObjectSystem> {
   public:
    constexpr static auto Type = _Value;
};

template <class _T, class ObjectSystem>
const bool Base<_T, ObjectSystem>::reg{
    ObjectSystem::instence().template register_class<_T>(
        [](_T &o) { std::println("called!"); })};
