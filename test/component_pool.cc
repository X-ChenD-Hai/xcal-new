#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

template <typename T, typename index_t, size_t max_cell_size = 256>
struct ComponentTraits {
    static_assert(sizeof(index_t) <= 16, "index_t must be 16 bytes or less");
    // utils
    template <auto val1, auto val2>
    static constexpr auto max = val1 > val2 ? val1 : val2;
    template <bool cond, auto val1, auto val2>
    static constexpr auto condition_if_v = cond ? val1 : val2;
    // type traits
    static constexpr bool is_trivially_destructible =
        std::is_trivially_destructible<T>::value;
    static constexpr bool is_trivially_copyable =
        std::is_trivially_copyable<T>::value;
    static constexpr bool is_trivially_moveable =
        std::is_trivially_move_constructible<T>::value;
    // sizeof
    static constexpr size_t comp_size = sizeof(T);
    static constexpr size_t index_size = sizeof(index_t);
    static constexpr size_t pointer_size = sizeof(T*);
    // alignment
    static constexpr size_t direct_cell_align = []() {
        if constexpr (comp_size == 1) {
            return 1;
        } else if constexpr (comp_size <= 2) {
            return 2;
        } else if constexpr (comp_size <= 4) {
            return 4;
        } else if constexpr (comp_size <= 8) {
            return 8;
        } else if constexpr (comp_size <= 16) {
            return 16;
        } else if constexpr (comp_size <= 32) {
            return 32;
        } else {
            return 64;
        }
    }();
    static_assert(direct_cell_align >= alignof(T),
                  "direct_cell_align must be at least alignof(T)");
    static constexpr size_t comp_cacheline_algn_size =
        condition_if_v<comp_size <= 64, direct_cell_align,
                       (comp_size / 64) * 64 +
                           condition_if_v<comp_size % 64 == 0, 0, 64>>;
    static constexpr size_t direct_cell_size =
        max<index_size, comp_cacheline_algn_size>;
    static constexpr size_t pointer_cell_align = max<index_size, pointer_size>;
    static constexpr size_t pointer_cell_size = pointer_cell_align;
    // store
    static constexpr bool use_direct_store =
        is_trivially_moveable && comp_size <= max_cell_size;

    // output
    static constexpr size_t cell_align =
        condition_if_v<use_direct_store, direct_cell_align, pointer_cell_align>;
    static constexpr size_t cell_size =
        condition_if_v<use_direct_store, direct_cell_size, pointer_cell_size>;
};

template <typename index_t, size_t cell_alogn, size_t cell_size>
struct alignas(cell_alogn) Cell {
    union {
        index_t next;
        unsigned char data[cell_size];
    };
};
template <typename index_t, size_t cell_alogn, size_t cell_size>
class BasePool {};

class ComponentPool {
   public:
    //    config
    static constexpr size_t cell_size = 8;
    static constexpr size_t cell_align = 8;
    static constexpr bool trivially_destructible = true;
    static constexpr bool use_direct_store = true;
    using index_t = uint32_t;
    static constexpr index_t default_pool_size = 1024;
    static constexpr std::align_val_t pool_memory_align{64};
    // impl
    using cell_t = Cell<index_t, cell_align, cell_size>;
    static constexpr index_t invalid_index =
        std::numeric_limits<index_t>::max();
    template <typename T>
    using traits = ComponentTraits<T, index_t>;
    template <typename T>
    static constexpr bool is_storeable =
        traits<T>::cell_align == cell_align &&
        traits<T>::cell_size == cell_size &&
        (trivially_destructible ? traits<T>::is_trivially_destructible
                                : !traits<T>::is_trivially_destructible) &&
        (use_direct_store ? traits<T>::use_direct_store
                          : !traits<T>::use_direct_store);
    cell_t* pool_data{nullptr};
    index_t free_list = invalid_index;
    index_t pool_size = 0;
    ComponentPool() { extend(default_pool_size); }

    void build_free_list(index_t from, index_t to) {
        for (index_t i = from; i < to; ++i) {
            pool_data[i].next = i + 1;
        }
        pool_data[to - 1].next = free_list;
        free_list = from;
    }

    void extend(index_t externd_size) {
        if (externd_size == 0) {
            return;
        }
        index_t old_size = pool_size;
        pool_size += externd_size;
        auto new_data = new (std::align_val_t(64)) cell_t[pool_size];
        memcpy(new_data, pool_data, cell_size * old_size);
        operator delete[](pool_data, pool_memory_align);
        pool_data = new_data;
        build_free_list(old_size, pool_size);
    }

    size_t allocate() {
        if (free_list == invalid_index) {
            extend(default_pool_size);
        }
        size_t id = free_list;
        free_list = pool_data[free_list].next;
        return id;
    }
    void deallocate(size_t id) {
        if (id == invalid_index) {
            return;
        }
        pool_data[id].next = free_list;
        free_list = id;
    }
    void* get(size_t id) {
        if (id == invalid_index) {
            return nullptr;
        }
        return &pool_data[id];
    }
    ~ComponentPool() { operator delete[](pool_data, pool_memory_align); }

    template <typename T>
        requires(is_storeable<T>)
    size_t allocate(T& comp) {
        size_t id = allocate();
        new (get(id)) T(comp);
        return id;
    }
    template <typename T>
        requires(is_storeable<T>)
    void deallocate(size_t id) {
        if (id == invalid_index) {
            return;
        }
        deallocate(id);
    }
    template <typename T>
        requires(is_storeable<T>)
    T* get(size_t id) {
        if (id == invalid_index) {
            return nullptr;
        }
        return static_cast<T*>(get(id));
    }
};
struct Com11 {
    uint32_t a;
    uint32_t a1;
};
TEST(ComponentPool, Cell) {
    using pool_t = ComponentPool;
    using cell_t = pool_t::cell_t;
    cell_t cell;
    cell.next = 1;
    EXPECT_EQ(cell.next, 1);
}
TEST(ComponentPool, Basic) {
    ComponentPool pool;
    auto comp = Com11{.a = 1, .a1 = 2};
    size_t id = pool.allocate(comp);
    EXPECT_NE(id, ComponentPool::invalid_index);
    auto comp1 = pool.get<Com11>(id);
    EXPECT_EQ(comp1->a, 1);
    EXPECT_EQ(comp1->a1, 2);
}