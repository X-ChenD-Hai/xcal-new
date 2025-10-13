#include <gtest/gtest.h>

#include <UniqueTypeTable.hpp>

// 定义一些空类作为测试用的类型标签
struct TestClassA {};
struct TestClassB {};
struct TestClassC {};
struct TestClassD {};

// 为每个测试套件使用唯一的Category标签，确保测试间静态变量完全隔离
struct TestCategory1 {};  // 用于基础功能测试
struct TestCategory2 {};  // 用于Get方法测试
struct TestCategory3 {};  // 用于默认值测试
struct TestCategory4 {};  // 用于数据向量测试
struct TestCategory5 {};  // 用于数据向量测试

// 测试套件：UniqueTypeTableTest
// 测试用例 1: 基本ID分配功能
TEST(UniqueTypeTableTest, UniqueIdAssignment) {
    using Cat = TestCategory1;
    using Table = UniqueTypeTable<Cat, int>;

    // 初始化静态变量（重要！）
    Table::default_data() = 0;
    
    size_t idA = Table::type_id<TestClassA>();
    size_t idB = Table::type_id<TestClassB>();
    size_t idC = Table::type_id<TestClassC>();

    // 验证ID是唯一且连续分配的
    EXPECT_EQ(idA, 0);
    EXPECT_EQ(idB, 1);
    EXPECT_EQ(idC, 2);
    EXPECT_NE(idA, idB);
    EXPECT_NE(idB, idC);
}

// 测试用例 2: Get方法的功能和引用可修改性
TEST(UniqueTypeTableTest, GetMethodReturnsCorrectReference) {
    using Cat = TestCategory2;  // 使用唯一的Category隔离
    using Table = UniqueTypeTable<Cat, int>;

    // 初始化静态变量
    Table::default_data() = 42;

    // !!! 修复关键：在获取引用前，先显式注册所有要使用的类型 !!!
    // 这会使data_向量resize到合适的大小，并初始化默认值
    Table::type_id<TestClassA>();
    Table::type_id<TestClassB>();

    // 现在再获取引用，因为data_大小已稳定，这些引用是安全的
    int& valueA = Table::data<TestClassA>();
    int& valueB = Table::data<TestClassB>();

    // 初始时应返回默认值
    EXPECT_EQ(valueA, 42);
    EXPECT_EQ(valueB, 42);

    // 通过引用修改值
    valueA = 100;
    valueB = 200;

    // 验证修改已持久化
    EXPECT_EQ(Table::data<TestClassA>(), 100);
    EXPECT_EQ(Table::data<TestClassB>(), 200);
    // 验证A和B的值是独立的
    EXPECT_NE(Table::data<TestClassA>(), Table::data<TestClassB>());
}
// 测试用例 3: 默认值变化的影响
TEST(UniqueTypeTableTest, DefaultValueChangeAffectsNewTypesOnly) {
    using Cat = TestCategory3;
    using Table = UniqueTypeTable<Cat, int>;

    Table::default_data() = 10;

    // 在默认值为10时注册ClassA
    size_t idA = Table::type_id<TestClassA>();
    EXPECT_EQ(Table::data<TestClassA>(), 10);

    // 更改默认值
    Table::default_data() = 20;

    // ClassA的值不应改变，因为它在默认值更改前已初始化
    EXPECT_EQ(Table::data<TestClassA>(), 10);

    // 新注册的ClassB应使用新的默认值
    size_t idB = Table::type_id<TestClassB>();
    EXPECT_EQ(Table::data<TestClassB>(), 20);
}

// 测试用例 5: 多次调用type_id返回相同ID
TEST(UniqueTypeTableTest, TypeIdIsConsistentForSameType) {
    using Cat = TestCategory5;
    using Table = UniqueTypeTable<Cat, int>;

    Table::default_data() = 0;

    size_t id1 = Table::type_id<TestClassA>();
    size_t id2 = Table::type_id<TestClassA>();
    size_t id3 = Table::type_id<TestClassA>();

    // 对同一类型的多次调用应返回相同的ID
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(id2, id3);
}

