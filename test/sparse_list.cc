#include <gtest/gtest.h>

#include <print>
#include <xc/common/sparse_list.hpp>

TEST(SparseList, Basic) {
    SparseList<uint32_t, uint32_t, 1024> list;
    uint32_t v = 1;
    auto k = list.insert(v);
    auto k1 = list.insert(22);
    list.insert(2222);

    std::println("list.has_value(v) = {}", list.has_value(v));
    std::println("list.get_value(k) = {}", list.get_value(k));
    std::println("list.get_value(k1) = {}", list.get_value(k1));
    std::println("list.get_index(k1) = {}", list.get_index(22));

    std::println("size = {}", list.size());
    list.insert(2222);
    std::println("bucket_count = {}", list.bucket_count());
    list.insert(4444);
    std::println("bucket_count = {}", list.bucket_count());
    list.insert(6666);
    std::println("bucket_count = {}", list.bucket_count());
    list.insert(8888);
    std::println("bucket_count = {}", list.bucket_count());
    list.insert(101010);
    std::println("bucket_count = {}", list.bucket_count());
}