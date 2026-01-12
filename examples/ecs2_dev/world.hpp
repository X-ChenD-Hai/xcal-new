#pragma once
#include <cstdint>
#include <type_traits>

namespace xc::ecs {
using entity_t = uint32_t;
using entity_id_t = uint32_t;
using entity_version_t = uint8_t;

namespace detail {
template <typename T>
struct EntityPropertiesImpl;
template <>
struct EntityPropertiesImpl<uint32_t> {
    static constexpr size_t IdBitCount = 24;
    static constexpr size_t VersionBitCount = 8;
    static_assert(IdBitCount + VersionBitCount <= sizeof(entity_t) * 8,
                  "IdBitCount + VersionBitCount must be less than or equal to "
                  "sizeof(entity_t) * 8");
    static constexpr entity_id_t MaxId = (1 << IdBitCount) - 1;
    static constexpr entity_version_t MaxVersion = ~entity_version_t(0);
    static constexpr entity_t IdMask = MaxId;
    static constexpr entity_t VersionMask = ((entity_t)MaxVersion)
                                            << IdBitCount;
    inline static constexpr entity_t entity(entity_id_t id,
                                            entity_version_t version) noexcept {
        return (entity_t(id) << VersionBitCount) | version;
    }
    inline static constexpr entity_id_t id(entity_t entity) noexcept {
        return entity & IdMask;
    }
    inline static constexpr entity_version_t version(entity_t entity) noexcept {
        return (entity & VersionMask) >> IdBitCount;
    }
};
template <>
struct EntityPropertiesImpl<uint64_t> {};
template <typename T>
struct EntityProperties : EntityPropertiesImpl<T> {
    static_assert(std::is_integral_v<T>,
                  "EntityProperties only supports integral types");
};

struct Entity {
    using properties = detail::EntityProperties<entity_t>;
    inline constexpr Entity(entity_id_t id, entity_version_t version) noexcept
        : entity_(properties::entity(id, version)) {}
    inline constexpr entity_id_t id() const noexcept {
        return properties::id(entity_);
    }
    inline constexpr entity_version_t version() const noexcept {
        return properties::version(entity_);
    }

   private:
    entity_t entity_;
};
}  // namespace detail


struct Table {};
template <typename... T>
struct TableStorage {};
struct Marker {};
struct System;
struct SystemPromise;
class SystemScheduler;
template <typename T>
struct Read {};
struct Yield {};
template <typename T>
struct Write {};
template <typename T>
struct ReadWrite {};
struct ChannelMarker {};
template <typename Fn, typename... Args>
struct Channel {};
template <typename T>
struct Where {};
template <typename... T>
struct Has {};

}  // namespace xc::ecs