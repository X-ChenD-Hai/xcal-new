#include <gtest/gtest.h>

#include <xc/ecs2/ecs.hpp>

TEST(Constants, Log2) {
    static_assert((xc::ecs::details::Log2<4>::value) == 2, "");
    static_assert((xc::ecs::details::Log2<5>::value) == 2, "");
    static_assert((xc::ecs::details::Log2<5>::mantissa) == 1, "");
    static_assert((xc::ecs::details::Log2<15>::value) == 3, "");
    static_assert((xc::ecs::details::Log2<15>::mantissa) == 7, "");
}

TEST(Constants, CellInfo) {
    struct Data1 {
        char s[5];
    };
    struct Data2 {
        char s[15];
    };
    struct Data3 {
        char s[9];
    };
    using c1 = xc::ecs::details::SlotInfo<size_t>;
    static_assert(c1::slot_size == 8, "");
    static_assert(c1::slot_grade == 3, "");
    static_assert(c1::object_size == 8, "");
    using c2 = xc::ecs::details::SlotInfo<Data1>;
    static_assert(c2::slot_size == 8, "");
    static_assert(c2::slot_grade == 3, "");
    static_assert(c2::object_size == 5, "");
    using c3 = xc::ecs::details::SlotInfo<Data2>;
    static_assert(c3::slot_size == 16, "");
    static_assert(c3::slot_grade == 4, "");
    static_assert(c3::object_size == 15, "");
    using c4 = xc::ecs::details::SlotInfo<Data3>;
    static_assert(c4::slot_size == 16, "");
    static_assert(c4::slot_grade == 4, "");
    static_assert(c4::object_size == 9, "");
}