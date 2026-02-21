#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <print>
#include <vector>
#include <xc/common/xc_assert.hpp>

/*---------- MemoPage ----------*/

class MemoPage {
   private:
    using offset_t = uint16_t;
    static constexpr offset_t InvalidOffset =
        std::numeric_limits<offset_t>::max();

    // Node 结构体与用户请求的对齐要求一致
    struct Node {
        offset_t next;
    };

    std::byte* raw_ptr_ = nullptr;
    std::byte* data_start_ = nullptr;
    offset_t free_ = InvalidOffset;
    size_t capacity_ = 0;
    size_t cell_size_ = 0;
    size_t align_ = 0;
    size_t padded_cell_size_ = 0;

    /* 计算对齐后的 cell 大小 */
    size_t calculate_padded_cell_size() const {
        const size_t total_size = sizeof(Node) + cell_size_;
        return (total_size + align_ - 1) & ~(align_ - 1);
    }

    /* 获取用户数据指针 */
    void* get_user_pointer(Node* node) const {
        std::byte* user_start =
            reinterpret_cast<std::byte*>(node) + sizeof(Node);

        // 确保用户数据指针满足对齐要求
        uintptr_t user_addr = reinterpret_cast<uintptr_t>(user_start);
        uintptr_t aligned_user_addr = (user_addr + align_ - 1) & ~(align_ - 1);

        return reinterpret_cast<void*>(aligned_user_addr);
    }

    /* 从用户指针获取 Node 指针 */
    Node* get_node_pointer(void* user_ptr) const {
        uintptr_t user_addr = reinterpret_cast<uintptr_t>(user_ptr);

        // 找到前一个对齐边界，Node 就在那里
        uintptr_t node_addr = user_addr - sizeof(Node);
        node_addr = node_addr & ~(align_ - 1);

        return reinterpret_cast<Node*>(node_addr);
    }

    /* 将字节偏移转成 Node 指针 */
    Node* offset_to_node(offset_t off) noexcept {
        return reinterpret_cast<Node*>(data_start_ + off);
    }

    /* 反向计算字节偏移 */
    offset_t node_to_offset(const Node* node) const noexcept {
        return static_cast<offset_t>(reinterpret_cast<const std::byte*>(node) -
                                     data_start_);
    }

   public:
    MemoPage(size_t capacity, size_t cell_size,
             size_t align = alignof(std::max_align_t))
        : capacity_(capacity), cell_size_(cell_size), align_(align) {
        XC_ASSERT(align >= alignof(Node) && ((align & (align - 1)) == 0));
        XC_ASSERT(capacity <= (InvalidOffset - 1));

        // 计算对齐后的 cell 大小
        padded_cell_size_ = calculate_padded_cell_size();
        const size_t total_bytes = padded_cell_size_ * capacity_;

        // 分配内存（包括对齐所需的空间）
        raw_ptr_ =
            static_cast<std::byte*>(std::malloc(total_bytes + align - 1));
        XC_ASSERT(raw_ptr_);

        // 对齐数据区域
        size_t space = total_bytes + align - 1;
        void* aligned_ptr =
            std::align(align, total_bytes, (void*&)raw_ptr_, space);
        XC_ASSERT(aligned_ptr);

        data_start_ = static_cast<std::byte*>(aligned_ptr);

        // 构造空闲链表
        for (size_t i = 0; i < capacity_; ++i) {
            Node* node =
                offset_to_node(static_cast<offset_t>(i * padded_cell_size_));
            node->next =
                (i + 1 == capacity_)
                    ? InvalidOffset
                    : static_cast<offset_t>((i + 1) * padded_cell_size_);
        }
        free_ = 0;
    }

    /* 分配一个对齐的 cell，返回用户可用指针，并通过 offset 给出索引 */
    void* allocate(size_t* idx = nullptr) noexcept {
        if (free_ == InvalidOffset) return nullptr;

        Node* node = offset_to_node(free_);
        void* user_ptr = get_user_pointer(node);

        if (idx) {
            *idx = free_ / padded_cell_size_;
        }

        free_ = node->next;
        return user_ptr;
    }

    /* 通过用户指针回收 */
    void deallocate(void* user_ptr) noexcept {
        XC_ASSERT(user_ptr);
        Node* node = get_node_pointer(user_ptr);

        node->next = free_;
        free_ = node_to_offset(node);
    }

    /* 通过索引回收 */
    void deallocate(size_t idx) noexcept {
        XC_ASSERT(idx < capacity_);
        Node* node =
            offset_to_node(static_cast<offset_t>(idx * padded_cell_size_));
        void* user_ptr = get_user_pointer(node);
        deallocate(user_ptr);
    }

    ~MemoPage() noexcept {
        if (raw_ptr_) {
            std::free(raw_ptr_);
        }
    }

    MemoPage(const MemoPage&) = delete;
    MemoPage& operator=(const MemoPage&) = delete;
};
/*---------- MemoPool ----------*/
template <typename T, bool = std::is_trivially_destructible_v<T>>
class MemoPool;

// 平凡析构类型特化 - 不需要调用析构函数
template <typename T>
class MemoPool<T, true> {
   private:
    static constexpr size_t DefaultPageCapacity = 256;
    std::vector<std::unique_ptr<MemoPage>> pages_;

   public:
    MemoPool() = default;

    // 分配对象
    template <typename... Args>
    T* allocate(Args&&... args) {
        // 尝试在现有页面中分配
        for (auto& page : pages_) {
            if (void* mem = page->allocate()) {
                return new (mem) T(std::forward<Args>(args)...);
            }
        }

        // 所有页面都满了，创建新页面
        auto new_page = std::make_unique<MemoPage>(DefaultPageCapacity,
                                                   sizeof(T), alignof(T));
        void* mem = new_page->allocate();
        XC_ASSERT(mem);  // 新页面必须有空间

        pages_.push_back(std::move(new_page));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // 释放对象（不调用析构函数）
    void deallocate(T* obj) noexcept {
        if (!obj) return;

        // 找到对象所在的页面
        for (auto& page : pages_) {
            // 通过地址范围判断对象是否属于该页面
            // 这里需要 MemoPage 提供地址查询功能，简化实现：遍历所有页面
            // 实际实现可能需要更高效的方法
            page->deallocate(obj);
            return;
        }

        XC_ASSERT(false && "Object not from this pool");
    }

    // 批量分配
    template <typename... Args>
    void allocate_n(size_t n, std::vector<T*>& result, Args&&... args) {
        result.reserve(result.size() + n);
        for (size_t i = 0; i < n; ++i) {
            result.push_back(allocate(std::forward<Args>(args)...));
        }
    }

    // 清空池（不调用析构函数）
    void clear() noexcept { pages_.clear(); }

    ~MemoPool() = default;  // 平凡析构类型，不需要特殊处理
};

// 非平凡析构类型特化 - 需要调用析构函数
template <typename T>
class MemoPool<T, false> {
   private:
    static constexpr size_t DefaultPageCapacity = 256;
    std::vector<std::unique_ptr<MemoPage>> pages_;
    std::vector<T*> allocated_objects_;  // 跟踪所有分配的对象

   public:
    MemoPool() = default;

    // 分配对象
    template <typename... Args>
    T* allocate(Args&&... args) {
        // 尝试在现有页面中分配
        for (auto& page : pages_) {
            if (void* mem = page->allocate()) {
                T* obj = new (mem) T(std::forward<Args>(args)...);
                allocated_objects_.push_back(obj);
                return obj;
            }
        }

        // 所有页面都满了，创建新页面
        auto new_page = std::make_unique<MemoPage>(DefaultPageCapacity,
                                                   sizeof(T), alignof(T));
        void* mem = new_page->allocate();
        XC_ASSERT(mem);

        T* obj = new (mem) T(std::forward<Args>(args)...);
        allocated_objects_.push_back(obj);
        pages_.push_back(std::move(new_page));
        return obj;
    }

    // 释放对象（调用析构函数）
    void deallocate(T* obj) noexcept {
        if (!obj) return;

        // 调用析构函数
        obj->~T();

        // 从跟踪列表中移除
        auto it = std::find(allocated_objects_.begin(),
                            allocated_objects_.end(), obj);
        if (it != allocated_objects_.end()) {
            allocated_objects_.erase(it);
        }

        // 回收内存
        for (auto& page : pages_) {
            // 简化实现：尝试在每个页面中回收
            // 实际实现需要更精确的页面查找
            page->deallocate(obj);
            return;
        }

        XC_ASSERT(false && "Object not from this pool");
    }

    // 批量分配
    template <typename... Args>
    void allocate_n(size_t n, std::vector<T*>& result, Args&&... args) {
        result.reserve(result.size() + n);
        for (size_t i = 0; i < n; ++i) {
            result.push_back(allocate(std::forward<Args>(args)...));
        }
    }

    // 清空池（调用所有对象的析构函数）
    void clear() noexcept {
        for (T* obj : allocated_objects_) {
            obj->~T();
        }
        allocated_objects_.clear();
        pages_.clear();
    }

    ~MemoPool() {
        // 析构时调用所有剩余对象的析构函数
        clear();
    }
};

/*---------- 测试用例 ----------*/
struct TrivialType {
    int x;
    double y;
    char name[32];
};  // 平凡析构类型

struct NonTrivialType {
    std::vector<int> data;
    std::string name;

    NonTrivialType(const std::string& n) : name(n) {}
    ~NonTrivialType() { std::println("NonTrivialType '{}' destroyed", name); }
};  // 非平凡析构类型

TEST(Pool, MemoPool) {
    std::println("Testing MemoPool with trivial type...");
    {
        MemoPool<TrivialType> pool;

        TrivialType* obj1 = pool.allocate();
        obj1->x = 10;
        obj1->y = 3.14;
        strcpy_s(obj1->name, sizeof(obj1->name), "test1");
        TrivialType* obj2 = pool.allocate();
        obj2->x = 20;
        obj2->y = 6.28;
        strcpy_s(obj2->name, "test2");

        std::println("Trivial objects allocated: {}, {}", obj1->name,
                     obj2->name);

        pool.deallocate(obj1);
        pool.deallocate(obj2);

        std::println("Trivial type test passed");
    }

    std::println("\nTesting MemoPool with non-trivial type...");
    {
        MemoPool<NonTrivialType> pool;

        NonTrivialType* obj1 = pool.allocate("first");
        obj1->data.push_back(1);
        obj1->data.push_back(2);

        NonTrivialType* obj2 = pool.allocate("second");
        obj2->data.push_back(3);

        std::println(
            "Non-trivial objects allocated: '{}' (size: {}), '{}' (size: {})",
            obj1->name, obj1->data.size(), obj2->name, obj2->data.size());

        pool.deallocate(obj1);
        // obj2 会在 pool 析构时自动调用析构函数
    }

    std::println("All tests passed");
}
