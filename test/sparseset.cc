#include <gtest/gtest.h>

#include <algorithm>
#include <print>
#include <vector>
#include <xc_assert.hpp>
struct Entity {
    friend struct SparsetSet;
    using entity_t = uint64_t;
    using id_t = uint32_t;
    using version_t = uint32_t;
    static constexpr auto ID_MASK = entity_t(id_t(-1));
    static constexpr auto VERSION_OFFSET = sizeof(id_t) * 8;
    static constexpr auto VERSION_MASK = entity_t(version_t(-1))
                                         << VERSION_OFFSET;
    entity_t entity_;

    Entity(entity_t entity) : entity_(entity) {}
    Entity(id_t id, version_t version)
        : entity_((entity_t)id | (entity_t(version) << VERSION_OFFSET)) {}
    id_t id() const noexcept { return (id_t)(entity_ & ID_MASK); }
    version_t version() const noexcept {
        return (version_t)((VERSION_MASK & entity_) >> VERSION_OFFSET);
    }
    bool operator==(const Entity& other) const noexcept {
        return entity_ == other.entity_;
    }

   protected:
    void increment_version() { entity_ += entity_t(1) << VERSION_OFFSET; }
};

template <>
struct std::formatter<Entity> {
    // 解析格式化字符串
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();
        // 简单的解析示例，这里假设格式化字符串中有一个 'e' 表示我们要格式化
        // Entity
        if (it != end && *it == 'e') {
            ++it;
        }
        if (it != end && *it != '}') {
            throw std::format_error("invalid format");
        }
        return it;
    }
    // 格式化 Entity 对象
    auto format(const Entity& entity, std::format_context& ctx) const {
        // 这里假设我们要将 Entity 对象格式化为 "ID: <id>, Version: <version>"
        // 的形式
        return std::format_to(ctx.out(), "ID: {}, Version: {}", entity.id(),
                              entity.version());
    }
};

struct SparsetSet {
    using eneity_list_t = std::vector<Entity>;
    using sparse_list_t = std::vector<uint32_t>;

    eneity_list_t dense_{};
    sparse_list_t sparse_{};
    static constexpr uint32_t INVALID_INDEX = uint32_t(-1);
    SparsetSet() {}

    void insert(Entity entity) {
        auto id = entity.id();
        auto it = search_index(dense_.begin(), dense_.end(), entity);
        if (it != dense_.end() && it->id() == entity.id()) {
            XC_ASSERT(it->version() < entity.version());
            *it = entity;
            sparse_[id] = it - dense_.begin();
            return;
        }
        auto new_index = it - dense_.begin();
        dense_.insert(it, entity);
        rebuild_sparse(new_index);
    }
    void rebuild_sparse(size_t start_index = 0) {
        while (!dense_.empty() && dense_.back().id() == INVALID_INDEX) {
            sparse_.pop_back();
        }
        if (dense_.empty()) {
            sparse_.clear();
            return;
        }
        if (dense_.back().id() >= sparse_.size()) {
            sparse_.resize(dense_.back().id() + 1, INVALID_INDEX);
        }
        for (size_t i = start_index; i < dense_.size(); ++i) {
            auto& entity = dense_[i];
            sparse_[entity.id()] = i;
        }
    }
    void insert_range(const eneity_list_t::iterator begin,
                      eneity_list_t::iterator end, bool is_sorted = false) {
        if (end == begin) return;

        if (!is_sorted) {
            // 非排序版本：需要复制和排序
            eneity_list_t to_insert(begin, end);
            std::sort(to_insert.begin(), to_insert.end(),
                      [](const Entity& a, const Entity& b) {
                          return a.id() < b.id();
                      });

            // 去重（相同ID保留版本号较大的）
            auto last = std::unique(to_insert.begin(), to_insert.end(),
                                    [](const Entity& a, const Entity& b) {
                                        return a.id() == b.id();
                                    });
            to_insert.erase(last, to_insert.end());

            // 合并到dense_中
            eneity_list_t result;
            result.reserve(dense_.size() + to_insert.size());

            auto dense_it = dense_.begin();
            auto insert_it = to_insert.begin();

            while (dense_it != dense_.end() && insert_it != to_insert.end()) {
                if (dense_it->id() < insert_it->id()) {
                    result.push_back(*dense_it);
                    ++dense_it;
                } else if (dense_it->id() > insert_it->id()) {
                    result.push_back(*insert_it);
                    ++insert_it;
                } else {
                    // ID相同，选择版本号较大的
                    if (insert_it->version() > dense_it->version()) {
                        result.push_back(*insert_it);
                    } else {
                        result.push_back(*dense_it);
                    }
                    ++dense_it;
                    ++insert_it;
                }
            }

            // 添加剩余元素
            result.insert(result.end(), dense_it, dense_.end());
            result.insert(result.end(), insert_it, to_insert.end());

            dense_ = std::move(result);
        } else {
            // 排序版本：直接使用输入范围，无需复制
            eneity_list_t result;
            result.reserve(dense_.size() + std::distance(begin, end));

            auto dense_it = dense_.begin();
            auto insert_it = begin;

            while (dense_it != dense_.end() && insert_it != end) {
                if (dense_it->id() < insert_it->id()) {
                    result.push_back(*dense_it);
                    ++dense_it;
                } else if (dense_it->id() > insert_it->id()) {
                    result.push_back(*insert_it);
                    ++insert_it;
                } else {
                    // ID相同，选择版本号较大的
                    if (insert_it->version() > dense_it->version()) {
                        result.push_back(*insert_it);
                    } else {
                        result.push_back(*dense_it);
                    }
                    ++dense_it;
                    ++insert_it;
                }
            }

            // 添加剩余元素
            result.insert(result.end(), dense_it, dense_.end());
            result.insert(result.end(), insert_it, end);

            dense_ = std::move(result);
        }

        rebuild_sparse();
    }

    eneity_list_t::iterator search_index(eneity_list_t::iterator begin,
                                         eneity_list_t::iterator end,
                                         Entity entity) {
        eneity_list_t::iterator result = end;  // 初始化结果为end，表示未找到
        while (begin < end) {
            auto it = begin + std::distance(begin, end) / 2;
            if (it->id() < entity.id()) {
                begin = it + 1;  // 目标在右侧
            } else {
                result = it;  // 找到一个大于等于id的元素，记录下来
                end = it;     // 继续在左侧查找更小的值
            }
        }
        return result;  // 返回记录的最小值
    }
    // 或者更高效的版本：使用 erase-remove 惯用法
    void remove_range(const std::vector<Entity>& entities) {
        if (entities.empty()) return;

        // 第一步：标记所有要删除的实体在 sparse_ 中为 INVALID_INDEX
        for (const auto& entity : entities) {
            auto id = entity.id();
            if (id < sparse_.size() &&
                entity.version() >= dense_[sparse_[id]].version()) {
                sparse_[id] = INVALID_INDEX;
            }
        }

        // 第二步：从 dense_ 中移除被标记的实体
        auto new_end = std::remove_if(
            dense_.begin(), dense_.end(), [this](const Entity& entity) {
                return sparse_[entity.id()] == INVALID_INDEX;
            });
        dense_.erase(new_end, dense_.end());

        // 第三步：完全重建 sparse_ 索引
        rebuild_sparse();
    }
    void remove_range(eneity_list_t::iterator begin,
                      eneity_list_t::iterator end) {
        if (begin == end) return;

        // 标记要删除的实体
        for (auto it = begin; it != end; ++it) {
            auto id = it->id();
            if (id < sparse_.size() &&
                it->version() >= dense_[sparse_[id]].version()) {
                sparse_[id] = INVALID_INDEX;
            }
        }

        // 从 dense_ 中移除
        dense_.erase(begin, end);

        // 重建索引
        rebuild_sparse();
    }
    void remove(Entity entity) { remove_range({entity}); }
    bool contains(Entity entity) const {
        auto id = entity.id();
        return (id < sparse_.size()) && sparse_[id] != INVALID_INDEX &&
               dense_[sparse_[id]] == entity;
    }
};

struct Component {
    using entity_t = Entity::entity_t;
    using id_t = Entity::id_t;
    using version_t = Entity::version_t;
    SparsetSet sparset_set_;
    using Cell = std::unique_ptr<void*, std::function<void(void*)>>;
    std::vector<Cell> cells_;
    SparsetSet entity_set_;
};

TEST(Entity, Construct) {
    Entity e(1, 0);
    EXPECT_EQ(1, e.id());
    EXPECT_EQ(0, e.version());
    Entity e1(3, 11);
    EXPECT_EQ(3, e1.id());
    EXPECT_EQ(11, e1.version());
}
TEST(SparseSetTest, Basic) {
    Entity e(1, 0);

    SparsetSet s;
    s.insert(e);
    EXPECT_TRUE(s.contains(e));
    s.remove(e);
    EXPECT_FALSE(s.contains(e));
    s.insert({1, 4});
    s.insert({4, 4});
    s.insert({5, 4});
    s.insert({3, 4});
    s.insert({2, 4});
    EXPECT_TRUE(s.contains({1, 4}));
    EXPECT_TRUE(s.contains({2, 4}));
    EXPECT_TRUE(s.contains({3, 4}));
    EXPECT_TRUE(s.contains({4, 4}));
    EXPECT_TRUE(s.contains({5, 4}));
    EXPECT_FALSE(s.contains(e));
}
TEST(SparsetSet, search_index) {
    SparsetSet s;
    s.insert(Entity(1, 0));
    s.insert(Entity(2, 0));
    s.insert(Entity(3, 0));
    s.insert(Entity(4, 0));
    s.insert(Entity(5, 0));
    s.insert(Entity(6, 0));
    s.insert(Entity(7, 0));
    s.insert(Entity(8, 0));
    s.insert(Entity(9, 0));
    s.insert(Entity(10, 0));
    s.insert(Entity(11, 0));
    s.insert(Entity(12, 0));

    auto it = s.search_index(s.dense_.begin(), s.dense_.end(), Entity(5, 0));
    EXPECT_EQ(4, it - s.dense_.begin());
}
TEST(SparsetSet, insert_range_strict_ordering) {
    SparsetSet s;
    SparsetSet s1;

    // 初始数据
    s.insert({1, 1});
    s.insert({2, 1});
    s.insert({5, 1});
    s.insert({6, 1});
    s.insert({9, 1});

    // 要插入的数据（故意乱序）
    s1.insert({4, 1});
    s1.insert({10, 1});
    s1.insert({3, 1});
    s1.insert({7, 1});
    s1.insert({0, 1});  // 测试边界情况
    s1.insert({8, 1});
    s1.insert({12, 1});

    s.insert_range(s1.dense_.begin(), s1.dense_.end());

    // 验证所有实体都存在
    EXPECT_TRUE(s.contains({0, 1}));
    EXPECT_TRUE(s.contains({1, 1}));
    EXPECT_TRUE(s.contains({2, 1}));
    EXPECT_TRUE(s.contains({3, 1}));
    EXPECT_TRUE(s.contains({4, 1}));
    EXPECT_TRUE(s.contains({5, 1}));
    EXPECT_TRUE(s.contains({6, 1}));
    EXPECT_TRUE(s.contains({7, 1}));
    EXPECT_TRUE(s.contains({8, 1}));
    EXPECT_TRUE(s.contains({9, 1}));
    EXPECT_TRUE(s.contains({10, 1}));
    EXPECT_TRUE(s.contains({12, 1}));

    // 严格验证有序性
    EXPECT_TRUE(std::is_sorted(
        s.dense_.begin(), s.dense_.end(),
        [](const Entity& a, const Entity& b) { return a.id() < b.id(); }));

    // 验证每个位置的ID
    EXPECT_EQ(s.dense_[0].id(), 0);
    EXPECT_EQ(s.dense_[1].id(), 1);
    EXPECT_EQ(s.dense_[2].id(), 2);
    EXPECT_EQ(s.dense_[3].id(), 3);
    EXPECT_EQ(s.dense_[4].id(), 4);
    EXPECT_EQ(s.dense_[5].id(), 5);
    EXPECT_EQ(s.dense_[6].id(), 6);
    EXPECT_EQ(s.dense_[7].id(), 7);
    EXPECT_EQ(s.dense_[8].id(), 8);
    EXPECT_EQ(s.dense_[9].id(), 9);
    EXPECT_EQ(s.dense_[10].id(), 10);
    EXPECT_EQ(s.dense_[11].id(), 12);

    // 验证稀疏索引正确性
    for (size_t i = 0; i < s.dense_.size(); ++i) {
        EXPECT_EQ(s.sparse_[s.dense_[i].id()], i);
    }
}
// 测试重复ID的情况（版本号处理）
TEST(SparsetSet, insert_range_version_handling) {
    SparsetSet s;
    SparsetSet s1;

    // 初始数据
    s.insert({1, 1});
    s.insert({2, 1});
    s.insert({3, 1});

    // 插入包含重复ID但不同版本的数据
    s1.insert({2, 5});  // 更高版本
    s1.insert({3, 0});  // 更低版本
    s1.insert({4, 3});  // 新ID

    s.insert_range(s1.dense_.begin(), s1.dense_.end());

    // 验证版本号处理正确
    EXPECT_TRUE(s.contains({2, 5}));  // 应该使用更高版本
    EXPECT_TRUE(s.contains({3, 1}));  // 应该保留原始更高版本
    EXPECT_TRUE(s.contains({4, 3}));  // 新ID应该存在

    EXPECT_FALSE(s.contains({2, 1}));  // 旧版本应该被替换
    EXPECT_FALSE(s.contains({3, 0}));  // 低版本不应该存在

    // 验证有序性
    EXPECT_TRUE(std::is_sorted(
        s.dense_.begin(), s.dense_.end(),
        [](const Entity& a, const Entity& b) { return a.id() < b.id(); }));
}
// 测试空范围和边界情况
TEST(SparsetSet, insert_range_edge_cases) {
    SparsetSet s;

    // 测试空范围
    std::vector<Entity> empty;
    s.insert_range(empty.begin(), empty.end());
    EXPECT_TRUE(s.dense_.empty());

    // 测试单个元素
    std::vector<Entity> single = {{5, 1}};
    s.insert_range(single.begin(), single.end());
    EXPECT_EQ(s.dense_.size(), 1);
    EXPECT_EQ(s.dense_[0].id(), 5);

    // 测试已排序输入
    std::vector<Entity> sorted = {{1, 1}, {3, 1}, {7, 1}};
    s.insert_range(sorted.begin(), sorted.end(), true);
    EXPECT_TRUE(std::is_sorted(
        s.dense_.begin(), s.dense_.end(),
        [](const Entity& a, const Entity& b) { return a.id() < b.id(); }));
}
TEST(SparsetSet, rebuild_sparse_optimized) {
    SparsetSet s;

    // 测试空情况
    s.rebuild_sparse();
    EXPECT_TRUE(s.sparse_.empty());

    // 测试插入后重建
    s.insert({1, 1});
    s.insert({5, 1});
    s.insert({3, 1});

    // 验证 dense_ 确实有序
    EXPECT_EQ(s.dense_[0].id(), 1);
    EXPECT_EQ(s.dense_[1].id(), 3);
    EXPECT_EQ(s.dense_[2].id(), 5);

    // 验证 sparse_ 正确
    EXPECT_EQ(s.sparse_.size(), 6);  // 0-5
    EXPECT_EQ(s.sparse_[1], 0u);
    EXPECT_EQ(s.sparse_[3], 1u);
    EXPECT_EQ(s.sparse_[5], 2u);

    // 测试部分重建
    s.rebuild_sparse(1);          // 只从索引1开始重建
    EXPECT_EQ(s.sparse_[1], 0u);  // 索引0保持不变
    EXPECT_EQ(s.sparse_[3], 1u);  // 索引1重新建立
    EXPECT_EQ(s.sparse_[5], 2u);  // 索引2重新建立
}

TEST(SparsetSet, remove_range) {
    SparsetSet s;

    // 初始化数据
    for (uint32_t i = 1; i <= 10; ++i) {
        s.insert({i, 1});
    }

    // 批量删除
    std::vector<Entity> to_remove = {{2, 1}, {5, 1}, {7, 1}, {9, 1}};
    s.remove_range(to_remove);

    // 验证删除结果
    EXPECT_FALSE(s.contains({2, 1}));
    EXPECT_FALSE(s.contains({5, 1}));
    EXPECT_FALSE(s.contains({7, 1}));
    EXPECT_FALSE(s.contains({9, 1}));

    // 验证剩余实体
    EXPECT_TRUE(s.contains({1, 1}));
    EXPECT_TRUE(s.contains({3, 1}));
    EXPECT_TRUE(s.contains({4, 1}));
    EXPECT_TRUE(s.contains({6, 1}));
    EXPECT_TRUE(s.contains({8, 1}));
    EXPECT_TRUE(s.contains({10, 1}));

    // 验证有序性
    EXPECT_TRUE(std::is_sorted(
        s.dense_.begin(), s.dense_.end(),
        [](const Entity& a, const Entity& b) { return a.id() < b.id(); }));

    // 验证索引正确性
    for (size_t i = 0; i < s.dense_.size(); ++i) {
        EXPECT_EQ(s.sparse_[s.dense_[i].id()], i);
    }
}

TEST(SparsetSet, remove_range_iterator) {
    SparsetSet s;

    for (uint32_t i = 1; i <= 8; ++i) {
        s.insert({i, 1});
    }

    // 删除 ID 为 3,4,5 的实体
    auto begin = std::find_if(s.dense_.begin(), s.dense_.end(),
                              [](const Entity& e) { return e.id() == 3; });
    auto end = std::find_if(s.dense_.begin(), s.dense_.end(),
                            [](const Entity& e) { return e.id() == 6; });

    s.remove_range(begin, end);

    // 验证结果
    EXPECT_TRUE(s.contains({1, 1}));
    EXPECT_TRUE(s.contains({2, 1}));
    EXPECT_FALSE(s.contains({3, 1}));
    EXPECT_FALSE(s.contains({4, 1}));
    EXPECT_FALSE(s.contains({5, 1}));
    EXPECT_TRUE(s.contains({6, 1}));
    EXPECT_TRUE(s.contains({7, 1}));
    EXPECT_TRUE(s.contains({8, 1}));
}