#pragma once
#include <malloc.h>
#include <vcruntime_typeinfo.h>

#include <array>
#include <cstdlib>
#include <ranges>
#include <unordered_map>

namespace internal {
template <typename T>
struct EventTraits {
    static constexpr auto required =
        std::is_trivially_destructible_v<T> &&
        std::is_trivially_copy_constructible_v<T> && sizeof(T) <= 64;
};
}  // namespace internal

class EventBus {
    std::array<std::byte *, 6> events_;
    std::array<uint16_t, 6> next_free_slots_;
    std::array<uint16_t, 6> chunk_size{};
    std::unordered_map<size_t, std::vector<uint16_t>> event_map_;
    static constexpr std::array<uint16_t, 64> size2index =
        []<size_t... I>(std::index_sequence<I...>) {
            return std::array<uint16_t, 64>{
                [](uint16_t size) consteval -> uint16_t {
                    uint16_t idx = 0;
                    while (size /= 2) {
                        idx++;
                    }
                    return idx ? idx : 1;
                }(I)...};
        }(std::make_index_sequence<64>{});

   public:
    EventBus() {
        for (auto [idx, event_ptr] : std::views::enumerate(events_)) {
            const auto alignment = (0x02 << idx);
            // std::println("insert size : {} ,index : {} ", alignment, idx);
#ifdef _MSC_VER
            event_ptr =
                (std::byte *)_aligned_malloc(alignment * 1024, alignment);
#else
            event_ptr =
                (std::byte *)std::aligned_alloc(alignment * 8, alignment);
#endif
            for (uint16_t i = 0; i < 1024; i++) {
                *((uint16_t *)(event_ptr + (i * alignment))) = i + 1;
            }
            *(uint16_t *)(event_ptr + (1023 * alignment)) = -1;
            next_free_slots_[idx] = 0;
            chunk_size[idx] = 1024;
        }
    }

    ~EventBus() {
        for (auto event_ptr : events_) {
#ifdef _MSC_VER
            _aligned_free(event_ptr);
#else
            std::free(event_ptr);
#endif
        }
    }

    template <typename T, typename... Args>
        requires(internal::EventTraits<T>::required)
    void publish(Args &&...args) {
        static constexpr auto index = size2index[sizeof(T)] - 1;
        static constexpr auto alignment = 0x02 << index;
        // std::println("size : {} ,index : {} ", sizeof(T), index);
        if (next_free_slots_[index] == uint16_t(-1)) {
            // std::println("extend chunk");
            extend_chunk(index);
        }
        auto ptr = (events_[index] + next_free_slots_[index] * alignment);
        if (event_map_.find(typeid(T).hash_code()) == event_map_.end())
            event_map_[typeid(T).hash_code()] = std::vector<uint16_t>();
        event_map_[typeid(T).hash_code()].push_back(next_free_slots_[index]);
        next_free_slots_[index] = *(uint16_t *)(ptr);
        new ((void *)ptr) T(std::forward<Args>(args)...);
    }

    void extend_chunk(const uint16_t index) {
        const auto alignment = 0x02 << index;
#ifdef _MSC_VER
        auto chunk = (std::byte *)_aligned_malloc(
            alignment * chunk_size[index] * 2, alignment);
        if (chunk == nullptr) {
            throw std::bad_alloc();
        }
        memcpy(chunk, events_[index], chunk_size[index] * alignment);
        _aligned_free(events_[index]);
#endif
        auto nsize = chunk_size[index] * 2;
        events_[index] = chunk;
        for (uint16_t i = chunk_size[index]; i < nsize; i++) {
            *((uint16_t *)(events_[index] + (i * alignment))) = i + 1;
        }
        *((uint16_t *)(events_[index] + ((nsize - 1) * alignment))) =
            uint16_t(-1);
        next_free_slots_[index] = chunk_size[index];
        chunk_size[index] = nsize;
    }

    template <typename Event, typename Fn, typename... Args>
        requires(std::is_invocable_v<Fn, Event &, Args && ...> &&
                 internal::EventTraits<Event>::required)
    void each(Fn fn, Args &&...args) {
        static constexpr auto index = size2index[sizeof(Event)] - 1;
        static constexpr auto alignment = 0x02 << index;

        if (event_map_.find(typeid(Event).hash_code()) == event_map_.end())
            return;
        auto &indices = event_map_[typeid(Event).hash_code()];
        if constexpr (std::is_invocable_r_v<bool, Fn, Event &, Args &&...>) {
            std::vector<uint16_t> new_indices;
            new_indices.reserve(indices.size());
            auto free_tmp = next_free_slots_[index];
            for (auto idx : indices) {
                auto ptr = (Event *)(events_[index] + (idx * alignment));
                if (fn(*ptr, std::forward<Args>(args)...)) {
                    *(uint16_t *)(events_[index] + (idx * alignment)) =
                        free_tmp;
                    free_tmp = idx;
                } else {
                    new_indices.push_back(idx);
                }
            }
            next_free_slots_[index] = free_tmp;
            indices = std::move(new_indices);
        } else {
            for (auto idx : indices) {
                auto ptr = (Event *)(events_[index] + (idx * alignment));
                fn(*ptr, std::forward<Args>(args)...);
            }
        }
    }

    template <typename Event>
        requires(internal::EventTraits<Event>::required)
    void clear() {
        static constexpr auto index = size2index[sizeof(Event)] - 1;
        static constexpr auto alignment = 0x02 << index;
        auto &indices = event_map_[typeid(Event).hash_code()];
        if (indices.empty()) {
            return;
        }
        auto free_tmp = next_free_slots_[index];
        for (auto &idx : indices) {
            *(uint16_t *)(events_[index] + (idx * alignment)) = free_tmp;
            free_tmp = idx;
        }
        next_free_slots_[index] = free_tmp;
        indices.clear();
    }
};