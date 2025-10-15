#include <gtest/gtest.h>

#include <ecs/Resource.hpp>
using namespace ecs;
TEST(ResourceManagerTest, Test1) {
    ResourceManager rm;

    EXPECT_FALSE(rm.has<int>());
    rm.add<int>(11);
    EXPECT_TRUE(rm.has<int>());
    EXPECT_EQ(rm.get<int>(), 11);
    ecs::ResourceManager rm2;
    EXPECT_FALSE(rm2.has<int>());
    rm2.add<int>(122);
    EXPECT_TRUE(rm2.has<int>());
    EXPECT_EQ(rm2.get<int>(), 122);
    EXPECT_EQ(rm.get<int>(), 11);
}

TEST(ResourceManagerTest, BasicResourceManagement) {
    ResourceManager rm;
    
    // 测试添加资源
    EXPECT_FALSE(rm.has<int>());
    rm.add<int>(42);
    EXPECT_TRUE(rm.has<int>());
    EXPECT_EQ(rm.get<int>(), 42);
    
    // 测试获取指针
    int* intPtr = rm.try_get<int>();
    ASSERT_NE(intPtr, nullptr);
    EXPECT_EQ(*intPtr, 42);
}

// 测试不同类型的资源
TEST(ResourceManagerTest, MultipleResourceTypes) {
    ResourceManager rm;
    
    // 添加多种类型的资源
    rm.add<int>(100);
    rm.add<double>(3.14);
    rm.add<std::string>("Hello World");
    
    EXPECT_TRUE(rm.has<int>());
    EXPECT_TRUE(rm.has<double>());
    EXPECT_TRUE(rm.has<std::string>());
    
    EXPECT_EQ(rm.get<int>(), 100);
    EXPECT_DOUBLE_EQ(rm.get<double>(), 3.14);
    EXPECT_EQ(rm.get<std::string>(), "Hello World");
}

// 测试资源删除
TEST(ResourceManagerTest, ResourceRemoval) {
    ResourceManager rm;
    
    rm.add<int>(50);
    EXPECT_TRUE(rm.has<int>());
    
    // 删除资源
    rm.remove<int>();
    EXPECT_FALSE(rm.has<int>());
    
    // 尝试获取已删除的资源
    EXPECT_EQ(rm.try_get<int>(), nullptr);
}

// 测试重复设置保护
TEST(ResourceManagerTest, DuplicateResourceProtection) {
    ResourceManager rm;
    
    rm.add<int>(10);
    EXPECT_TRUE(rm.has<int>());
    EXPECT_EQ(rm.get<int>(), 10);
    
    // 尝试重复设置应该被阻止
    testing::internal::CaptureStderr(); // 捕获 stderr 输出
    rm.add<int>(20);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_FALSE(output.empty()); // 应该有错误消息
    
    // 原始值应该保持不变
    EXPECT_EQ(rm.get<int>(), 10);
}

// 测试删除不存在的资源
TEST(ResourceManagerTest, RemoveNonExistentResource) {
    ResourceManager rm;
    
    testing::internal::CaptureStderr();
    rm.remove<int>(); // 尝试删除不存在的资源
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_FALSE(output.empty()); // 应该有错误消息
}

// 测试获取不存在的资源
TEST(ResourceManagerTest, GetNonExistentResource) {
    ResourceManager rm;
    
    // 测试 try_get 对不存在的资源
    EXPECT_EQ(rm.try_get<int>(), nullptr);
    
    // 测试 get() 对不存在的资源 - 这可能会崩溃，所以需要小心
    // 根据您的实现，可能需要添加边界检查
}

// 测试 const 版本的方法
TEST(ResourceManagerTest, ConstMethods) {
    ResourceManager rm;
    rm.add<int>(123);
    
    const ResourceManager& const_rm = rm;
    
    EXPECT_TRUE(const_rm.has<int>());
    EXPECT_EQ(const_rm.get<int>(), 123);
    
    const int* constPtr = const_rm.try_get<int>();
    ASSERT_NE(constPtr, nullptr);
    EXPECT_EQ(*constPtr, 123);
}

// 测试自定义类型
TEST(ResourceManagerTest, CustomTypes) {
    struct Point {
        int x, y;
        Point(int x, int y) : x(x), y(y) {}
    };
    
    ResourceManager rm;
    rm.add<Point>(10, 20);
    
    EXPECT_TRUE(rm.has<Point>());
    Point& point = rm.get<Point>();
    EXPECT_EQ(point.x, 10);
    EXPECT_EQ(point.y, 20);
}

// 测试指针资源（不管理内存的情况）
TEST(ResourceManagerTest, PointerResources) {
    ResourceManager rm;
    
    int* externalInt = new int(999);
    rm.add<int>(externalInt); // 使用指针版本
    
    EXPECT_TRUE(rm.has<int>());
    EXPECT_EQ(rm.get<int>(), 999);
    
    // 注意：由于使用了空的删除器，需要手动管理内存
    delete externalInt;
}

// 测试资源ID的唯一性
TEST(ResourceManagerTest, ResourceIdUniqueness) {
    ResourceManager rm1, rm2;
    
    // 每个 ResourceManager 实例应该有独立的资源映射
    rm1.add<int>(1);
    rm2.add<int>(2);
    
    EXPECT_EQ(rm1.get<int>(), 1);
    EXPECT_EQ(rm2.get<int>(), 2);
}

// 测试移动语义（如果支持）
TEST(ResourceManagerTest, ResourceManagerIsolation) {
    ResourceManager rm1;
    rm1.add<int>(100);
    rm1.add<double>(2.5);
    
    ResourceManager rm2;
    rm2.add<std::string>("Test");
    
    // 确保管理器之间是隔离的
    EXPECT_TRUE(rm1.has<int>());
    EXPECT_TRUE(rm1.has<double>());
    EXPECT_FALSE(rm1.has<std::string>());
    
    EXPECT_FALSE(rm2.has<int>());
    EXPECT_FALSE(rm2.has<double>());
    EXPECT_TRUE(rm2.has<std::string>());
}

// 测试异常安全性（如果适用）
TEST(ResourceManagerTest, ExceptionSafety) {
    ResourceManager rm;
    
    // 添加一些资源
    rm.add<int>(1);
    rm.add<double>(2.0);
    
    // 即使中间操作失败，已有资源应该保持有效
    // 这里可以测试资源管理器在异常情况下的稳定性
}


// 边界条件测试
TEST(ResourceManagerTest, EdgeCases) {
    ResourceManager rm;
    
    // 测试空资源管理器
    EXPECT_FALSE(rm.has<int>());
    EXPECT_EQ(rm.try_get<int>(), nullptr);
    
    // 添加然后立即删除
    rm.add<int>(42);
    rm.remove<int>();
    EXPECT_FALSE(rm.has<int>());
    
    // 重新添加
    rm.add<int>(84);
    EXPECT_TRUE(rm.has<int>());
    EXPECT_EQ(rm.get<int>(), 84);
}