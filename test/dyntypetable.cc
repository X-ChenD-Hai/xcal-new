#include <gtest/gtest.h>

#include <chrono>
#include <print>
#include <unordered_map>
#include <xc/common/sparse_list.hpp>
#include <xc/common/type_map.hpp>

// t[tid][cid]=data

TEST(DynTypeTable, get_type_id) {
    TypeIdGenerator table, table2;
    size_t type_id11 = table.type_id<int>();
    size_t type_id12 = table.type_id<double>();
    size_t type_id21 = table2.type_id<double>();
    size_t type_id22 = table2.type_id<int>();
    EXPECT_EQ(type_id11, 0);
    EXPECT_EQ(type_id11, table.type_id<int>());
    EXPECT_EQ(type_id12, 1);
    EXPECT_EQ(type_id12, table.type_id<double>());

    EXPECT_EQ(type_id21, 0);
    EXPECT_EQ(type_id21, table2.type_id<double>());
    EXPECT_EQ(type_id22, 1);
    EXPECT_EQ(type_id22, table2.type_id<int>());
    std::unordered_map<size_t, size_t> c;
    c.insert({type_id11, 0});
    c.insert({type_id12, 1});
    const auto SEARCH_EPOCHE = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < SEARCH_EPOCHE; i++) {
        auto k = c[type_id11];
    }
    std::print(
        "searched by map: {}\n",
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            std::chrono::high_resolution_clock::now() - start) /
            SEARCH_EPOCHE);
    auto sl = SparseList<size_t, size_t, 1024>{};
    sl.insert(type_id11);
    sl.insert(type_id12);
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < SEARCH_EPOCHE; i++) {
        auto k = sl.get_index(type_id11);
    }
    std::print(
        "searched by sparselist: {}\n",
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            std::chrono::high_resolution_clock::now() - start) /
            SEARCH_EPOCHE);
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < SEARCH_EPOCHE; i++) {
        auto k = table.type_id<int>();
    }
    std::print(
        "searched by table: {}\n",
        std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
            std::chrono::high_resolution_clock::now() - start) /
            SEARCH_EPOCHE);
}
TEST(DynTypeMap, data) {
    auto table = TypeMap<int>{};
    table.data<int>() = 10;
    EXPECT_EQ(table.data<int>(), 10);
    table.data<double>() = 20;
    EXPECT_EQ(table.data<double>(), 20);
    EXPECT_EQ(table.data<float>(), -1);
}