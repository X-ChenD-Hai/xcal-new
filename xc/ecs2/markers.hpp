#pragma once
namespace xc::ecs {
template <typename... T>
class Derived;

template <typename... T>
class Resource;

template <typename T>
class Optional {
    using type = T;
};

template <typename... Eany>
struct ExcludeAny;

template <typename... Eall>
struct ExcludeAll;

template <typename... R>
struct Read;

template <typename... Rw>
struct ReadWrite;

template <typename... Rw>
struct CacheTag;

template <typename... T>
struct CreateEntity;

template <typename... T>
struct DestroyEntity;

template <typename... T>
struct Attach;

template <typename... T>
struct Detach;

};  // namespace xc::ecs