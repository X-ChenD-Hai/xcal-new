#pragma once
#include <malloc.h>

#include <cstdlib>
#include <type_map.hpp>
#include <xc_assert.hpp>

#include "./utils/traits.hpp"

namespace ecs {
namespace details {
template <size_t size, size_t alignment = 1, size_t log2_ = 0>
struct aligned_size {
    static constexpr size_t value =
        aligned_size<size / 2 + size % 2, alignment * 2, log2_ + 1>::value;
    static constexpr size_t log2 =
        aligned_size<size / 2 + size % 2, alignment * 2, log2_ + 1>::log2;
};
template <size_t alignment, size_t log2_>
struct aligned_size<1, alignment, log2_> {
    static constexpr size_t value = alignment;
    static constexpr size_t log2 = log2_;
};
}  // namespace details

struct EventBus final {
   private:
    template <size_t cell_size>
    class EventPool {
        friend class EventBus;

       private:
        template <size_t size>
        struct alignas(std::max(size, alignof(uint32_t))) Cell {
            union {
                uint32_t next;
                std::byte data[size];
            };
        };
        static_assert(std::is_trivially_copy_assignable_v<Cell<1>>, "");
        struct EventInfo {
            void (*move_callback)(void* src, void* dst){nullptr};
            void (*destroy_callback)(void*){nullptr};
            std::vector<uint32_t> cell_indices{};
        };
        template <typename Event>
            requires(std::is_move_constructible_v<Event>)
        struct EventInfoBuilder {
            inline static EventInfo info() {
                EventInfo info;
                if constexpr (std::is_trivially_move_constructible_v<Event>) {
                    info.move_callback = nullptr;
                } else {
                    info.move_callback = [](void* src, void* dst) {
                        std::construct_at((Event*)dst,
                                          std::move(*((Event*)src)));
                    };
                }
                if constexpr (std::is_trivially_destructible_v<Event>) {
                    info.destroy_callback = nullptr;
                } else {
                    info.destroy_callback = [](void* ptr) {
                        std::destroy_at((Event*)ptr);
                    };
                }
                return info;
            }
        };
        friend class EventBus;
        static constexpr uint32_t invalid_index =
            std::numeric_limits<uint32_t>::max();
        uint32_t pool_size_{0};
        uint32_t free_list_{invalid_index};
        std::vector<EventInfo> event_infos_;
        TypeMap<uint32_t> index_map_{invalid_index};
        Cell<cell_size>* cells_{nullptr};
        size_t size_{0};
        EventPool() {}

       public:
        void extend(size_t extend_size) {
            auto new_cells = new (std::align_val_t{64})
                Cell<cell_size>[pool_size_ + extend_size];
            if (cells_)
                memcpy(new_cells, cells_, pool_size_ * sizeof(Cell<cell_size>));
            for (auto idx : index_map_) {
                if (idx == invalid_index) continue;
                auto& info = event_infos_[idx];
                if (info.move_callback) {
                    for (auto& idx : info.cell_indices) {
                        info.move_callback(cells_[idx].data,
                                           new_cells[idx].data);
                    }
                } else {
                }
                if (info.destroy_callback) {
                    for (auto idx : info.cell_indices) {
                        info.destroy_callback(cells_[idx].data);
                    }
                }
            }
            for (size_t i = pool_size_; i < pool_size_ + extend_size - 1; i++) {
                new_cells[i].next = i + 1;
            }
            new_cells[pool_size_ + extend_size - 1].next = free_list_;
            free_list_ = pool_size_;
            pool_size_ += extend_size;
            if (cells_) operator delete[](cells_, std::align_val_t{64});
            cells_ = new_cells;
        }
        void shrink() {
            // 收缩条件：使用量小于当前池大小的1/4，且池大小大于最小限制
            constexpr uint32_t min_pool_size = 4;  // 最小保持4个单元
            if (size_ == 0 || pool_size_ <= min_pool_size ||
                size_ > pool_size_ / 4) {
                return;
            }

            // 计算新大小：不小于min_pool_size，不超过原大小1/2，且是2的幂次
            uint32_t new_size = std::max(
                min_pool_size,
                static_cast<uint32_t>(std::pow(2, std::log2(size_ * 2))));
            new_size = std::min(new_size, pool_size_ / 2);

            auto new_cells =
                new (std::align_val_t{64}) Cell<cell_size>[new_size];
            size_t dst_idx = 0;
            for (auto idx : index_map_) {
                if (idx == invalid_index) continue;
                auto info = event_infos_[idx];
                auto new_indices =
                    std::vector<uint32_t>(info.cell_indices.size());
                if (info.move_callback) {
                    for (auto src_idx : info.cell_indices) {
                        info.move_callback(cells_[src_idx].data,
                                           new_cells[dst_idx].data);
                        new_indices.push_back(dst_idx);
                        dst_idx++;
                    }
                } else {
                    for (auto src_idx : info.cell_indices) {
                        memcpy(new_cells[dst_idx].data, cells_[src_idx].data,
                               sizeof(Cell<cell_size>));
                        new_indices.push_back(dst_idx);
                        dst_idx++;
                    }
                }
                if (info.destroy_callback) {
                    for (auto src_idx : info.cell_indices) {
                        info.destroy_callback(cells_[src_idx].data);
                    }
                }
                info.cell_indices = new_indices;
            }
            free_list_ = dst_idx;
            for (; dst_idx < new_size - 1; dst_idx++) {
                new_cells[dst_idx].next = dst_idx + 1;
            }
            new_cells[new_size - 1].next = invalid_index;
            if (cells_) operator delete[](cells_, std::align_val_t{64});
            cells_ = new_cells;
            pool_size_ = new_size;
        }

        template <typename Event, typename... Args>
            requires(std::is_constructible_v<Event, Args...> &&
                     details::aligned_size<sizeof(Event)>::value == cell_size)
        void publish(Args&&... args) {
            if (free_list_ == invalid_index) {
                extend(pool_size_ ? pool_size_ : 1);
            }
            XC_ASSERT(free_list_ != invalid_index);
            auto& cell = cells_[free_list_];
            if (index_map_.data<Event>() == invalid_index) {
                event_infos_.emplace_back(EventInfoBuilder<Event>::info());
                index_map_.data<Event>() = event_infos_.size() - 1;
            }
            auto& indices = event_infos_[index_map_.data<Event>()].cell_indices;
            indices.emplace_back(free_list_);
            free_list_ = cell.next;
            new ((void*)cell.data) Event(std::forward<Args>(args)...);
            size_++;
        }

        inline void free_index(uint32_t index) {
            cells_[index].next = free_list_;
            free_list_ = index;
            size_--;
        }
        inline void free_indeices(const std::vector<uint32_t>& indices) {
            for (auto& ids : indices) {
                free_index(ids);
            }
        }

        template <typename Event, typename Fn, typename... Args>
            requires(std::is_invocable_v<Fn, Args...> ||
                     std::is_invocable_v<Fn, Event&, Args...> ||
                     std::is_invocable_v<Fn, const Event&, Args...>)
        bool each(Fn fn, Args&&... args) const noexcept {
            if (index_map_.data<Event>() == invalid_index) return false;
            if (event_infos_[index_map_.data<Event>()].cell_indices.empty())
                return false;
            for (auto idx :
                 event_infos_[index_map_.data<Event>()].cell_indices) {
                auto ptr = (Event*)(cells_[idx].data);
                // std::println("ptr {}", (void*)ptr);
                if constexpr (std ::is_invocable_v<Fn, Args&&...>)
                    fn(std::forward<Args>(args)...);
                else
                    fn(*ptr, std::forward<Args>(args)...);
            }
            return true;
        }
        template <typename Event, typename Fn, typename... Args>
            requires((std::is_invocable_v<Fn, Args && ...> ||
                      std::is_invocable_v<Fn, Event&, Args && ...>))
        bool each(Fn fn, Args&&... args) {
            if (index_map_.data<Event>() == invalid_index) return false;
            auto& cell_indices =
                event_infos_[index_map_.data<Event>()].cell_indices;
            if (cell_indices.empty()) return false;
            if constexpr (std::is_invocable_r_v<bool, Fn, Event&, Args&&...> ||
                          std::is_invocable_r_v<bool, Fn, Args&&...>) {
                std::vector<uint32_t> new_indices;
                new_indices.reserve(cell_indices.size());
                for (auto idx : cell_indices) {
                    auto ptr = (Event*)cells_[idx].data;
                    if constexpr (std ::is_invocable_v<Fn, Event&, Args&&...>) {
                        if (fn(*ptr, std::forward<Args>(args)...)) {
                            if constexpr (!std::is_trivially_destructible_v<
                                              Event>)
                                std::destroy_at(ptr);
                            free_index(idx);
                        } else {
                            new_indices.push_back(idx);
                        }
                    } else {
                        if (fn(std::forward<Args>(args)...)) {
                            if constexpr (!std::is_trivially_destructible_v<
                                              Event>)
                                std::destroy_at(ptr);
                            free_index(idx);
                        } else {
                            new_indices.push_back(idx);
                        }
                    }
                }
                cell_indices = new_indices;
            } else {
                const_cast<const EventPool*>(this)->each<Event>(
                    fn, std::forward<Args>(args)...);
            }
            return true;
        }
        template <typename T>
            requires(details::aligned_size<sizeof(T)>::value == cell_size)
        void clear() {
            if (index_map_.data<T>() == invalid_index) return;
            auto& cell_indices =
                event_infos_[index_map_.data<T>()].cell_indices;
            if (cell_indices.empty()) return;
            // std::cout << "clear " << typeid(T).name() << " " <<
            // cell_indices.size()
            //           << std::endl;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (auto idx : cell_indices) {
                    auto ptr = (T*)(cells_[idx].data);
                    std::destroy_at(ptr);
                }
            }
            free_indeices(cell_indices);
            cell_indices.clear();
        }
        void clear() {
            for (auto& p : index_map_) {
                if (p == invalid_index) continue;
                auto& cell_indices = event_infos_[p].cell_indices;
                if (event_infos_[p].destroy_callback) {
                    for (auto idx : cell_indices)
                        event_infos_[p].destroy_callback(cells_[idx].data);
                }
                free_indeices(cell_indices);
                cell_indices.clear();
            }
        }

        template <typename Event>
        inline size_t size() const noexcept {
            if (index_map_.data<Event>() == invalid_index) return 0;
            return event_infos_[index_map_.data<Event>()].cell_indices.size();
        }
        inline decltype(auto) cells() const noexcept { return cells_; }
        inline decltype(auto) event_infos() const noexcept {
            return event_infos_;
        }
        inline decltype(auto) index_map() const noexcept { return index_map_; }
        inline decltype(auto) free_list() const noexcept { return free_list_; }
        inline decltype(auto) pool_size() const noexcept { return pool_size_; }
        inline decltype(auto) size() const noexcept { return size_; }

        ~EventPool() {
            clear();
            if (cells_) operator delete[](cells_, std::align_val_t{64});
            pool_size_ = 0;
            free_list_ = invalid_index;
        }
    };
    template <typename Event>
    constexpr static auto index = details::aligned_size<sizeof(Event)>::log2;

    template <typename T>
        requires(sizeof(T) <= 64)
    friend class EventIterator;
    static constexpr size_t pool_size = 1024;
    static constexpr uint32_t invalid_index =
        std::numeric_limits<uint32_t>::max();
    std::tuple<EventPool<1>*, EventPool<2>*, EventPool<4>*, EventPool<8>*,
               EventPool<16>*, EventPool<32>*, EventPool<64>*>
        pools_;

   public:
    template <typename T>
    class EventRange;
    template <typename T>
        requires(sizeof(T) <= 64)
    class EventIterator {
        static constexpr auto index = details::aligned_size<sizeof(T)>::log2;
        EventPool<details::aligned_size<sizeof(T)>::value>& pool;
        std::vector<uint32_t>& indices_;
        size_t index_ = 0;

       public:
        T& operator*() const {
            return *(T*)(pool.cells_[indices_[index_]].data);
        }
        T* operator->() const {
            return (T*)(pool.cells_[indices_[index_]].data);
        }
        bool operator==(const EventIterator& other) const {
            return index_ == other.index_;
        }
        bool operator!=(const EventIterator& other) const {
            return index_ != other.index_;
        }
        void operator++() { index_++; }
        void operator++(int) { index_++; }
        void operator--() { index_--; }
        void operator--(int) { index_--; }

       protected:
        EventIterator(EventPool<details::aligned_size<sizeof(T)>::value>& pool,
                      std::vector<uint32_t>& indices, size_t index = -1)
            : pool(pool),
              indices_(indices),
              index_(index == -1 ? indices.size() : index) {}
        friend class EventRange<T>;
    };
    template <typename Event>
    class EventRange {
        EventPool<details::aligned_size<sizeof(Event)>::value>& pool_;
        std::vector<uint32_t>& indices_;

       protected:
        EventRange(EventPool<details::aligned_size<sizeof(Event)>::value>& pool,
                   std::vector<uint32_t>& indices)
            : pool_(pool), indices_(indices) {}

       public:
        EventIterator<Event> begin() const {
            return EventIterator<Event>(pool_, indices_, 0);
        }
        EventIterator<Event> end() const {
            return EventIterator<Event>(pool_, indices_, indices_.size());
        }
        friend class EventBus;
    };
    template <typename Event>
        requires(sizeof(Event) <= 64)
    EventRange<Event> each() const noexcept {
        auto& pool = std::get<index<Event>>(pools_);
        return EventRange<Event>(
            *pool, pool->event_infos_[pool->index_map_.template data<Event>()]
                       .cell_indices);
    }
    template <typename Event, typename Fn, typename... Args>
        requires((std::is_invocable_v<Fn, Args&...> ||
                  std::is_invocable_v<Fn, Event&, Args&...>))
    bool each(Fn fn, Args&&... args) {
        return std::get<index<Event>>(pools_)->template each<Event>(
            fn, std::forward<Args>(args)...);
    }
    template <typename Event, typename Fn, typename... Args>
        requires(std::is_invocable_v<Fn, Args&...> ||
                 std::is_invocable_v<Fn, const Event&, Args&...>)
    bool each(Fn fn, Args&&... args) const noexcept {
        return std::get<index<Event>>(pools_)->template each<Event>(
            fn, std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
        requires(
            std::is_invocable_v<Fn, first_arg_type_t<Fn>&, Args & ...> ||
            std::is_invocable_v<Fn, const first_arg_type_t<Fn>&, Args & ...>)
    inline bool each(Fn fn, Args&&... args) const noexcept {
        using Event = first_arg_type_of_t<fn>;
        return each<Event>(fn, std::forward<Args>(args)...);
    }
    template <typename Fn, typename... Args>
        requires(
            std::is_invocable_v<Fn, first_arg_type_t<Fn>&, Args & ...> ||
            std::is_invocable_v<Fn, const first_arg_type_t<Fn>&, Args & ...>)
    inline bool each(Fn fn, Args&&... args) {
        using Event = first_arg_type_of_t<fn>;
        return each<Event>(fn, std::forward<Args>(args)...);
    }

    template <typename... Event>
    bool all_exist() const noexcept {
        return (exist<Event>() && ...);
    }
    template <typename... Event>
    bool any_exist() const noexcept {
        return (exist<Event>() || ...);
    }
    template <typename Event>
    inline bool exist() const noexcept {
        return std::get<index<Event>>(pools_)
                       ->index_map_.template data<Event>() != invalid_index &&
               std::get<index<Event>>(pools_)
                       ->event_infos_[std::get<index<Event>>(pools_)
                                          ->index_map_.template data<Event>()]
                       .cell_indices.size() > 0;
    }

    EventBus() {
        []<size_t... I>(std::index_sequence<I...>, auto& pools) {
            (
                [](auto& pools) {
                    using type =
                        std::remove_reference_t<decltype(*std::get<I>(pools))>;
                    std::get<I>(pools) = new type();
                }(pools),
                ...);
        }(std::make_index_sequence<sizeof(pools_) / sizeof(nullptr)>(), pools_);
    }

    template <typename Event, typename... Args>
        requires(std::is_constructible_v<Event, Args...> && sizeof(Event) <= 64)
    inline void publish(Args&&... args) {
        std::get<index<Event>>(pools_)->template publish<Event>(
            std::forward<Args>(args)...);
    }

    template <typename T>
    inline void clear() {
        std::get<index<T>>(pools_)->template clear<T>();
    }
    inline void clear() {
        []<size_t... I>(std::index_sequence<I...>, auto& pools) {
            (
                [](auto& pools) {
                    using type =
                        std::remove_reference_t<decltype(*std::get<I>(pools))>;
                    std::get<I>(pools)->clear();
                }(pools),
                ...);
        }(std::make_index_sequence<sizeof(pools_) / sizeof(nullptr)>(), pools_);
    }

    ~EventBus() {
        []<size_t... I>(std::index_sequence<I...>, auto& pools) {
            (
                [](auto& pools) {
                    using type =
                        std::remove_reference_t<decltype(*std::get<I>(pools))>;
                    std::get<I>(pools)->clear();
                    delete std::get<I>(pools);
                }(pools),
                ...);
        }(std::make_index_sequence<sizeof(pools_) / sizeof(nullptr)>(), pools_);
    }

    inline size_t size() const noexcept {
        return []<size_t... I>(std::index_sequence<I...>, auto& pools) {
            return (std::get<I>(pools)->size() + ... + 0);
        }(std::make_index_sequence<sizeof(pools_) / sizeof(nullptr)>(), pools_);
    }
    template <typename Event>
    inline size_t size() const noexcept {
        return std::get<index<Event>>(pools_)->template size<Event>();
    }
    void shrink() {
        []<size_t... I>(std::index_sequence<I...>, auto& pools) {
            (
                [](auto& pools) {
                    using type =
                        std::remove_reference_t<decltype(*std::get<I>(pools))>;
                    std::get<I>(pools)->shrink();
                }(pools),
                ...);
        }(std::make_index_sequence<sizeof(pools_) / sizeof(nullptr)>(), pools_);
    }

    template <size_t I>
    decltype(auto) pool() {
        return *std::get<I>(pools_);
    }
    template <typename Event>
    decltype(auto) pool_of() {
        return *std::get<index<Event>>(pools_);
    }
    template <typename Event>
    static constexpr size_t pool_index() {
        return index<Event>;
    }
};

}  // namespace ecs
