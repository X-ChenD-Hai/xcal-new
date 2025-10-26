#include <gtest/gtest.h>

#include <concepts>
#include <crtp_base.hpp>

struct Base {
    float pos_x, pos_y, pos_z;
    float scale_x, scale_y, scale_z;
    float rotete_theta, rotate_x, rotate_y, rotate_z;
};

#define CRTP_CLASS(Derived)                                    \
   private:                                                    \
    T& self() const noexcept {                                 \
        return *const_cast<T*>(static_cast<const T*>((this))); \
    }

template <typename T>
struct Posable {
    CRTP_CLASS(T)
   public:
    float& pos_x() const noexcept { return self().base.pos_x; }
    float& pos_y() const noexcept { return self().base.pos_y; }
    float& pos_z() const noexcept { return self().base.pos_z; }

    auto& set_pos(float x, float y, float z) const noexcept {
        self().base.pos_x = x;
        self().base.pos_y = y;
        self().base.pos_z = z;
        return self();
    }
};
template <typename T>
struct Scaleable {
    CRTP_CLASS(T)
   public:
    auto& print_scale() const noexcept
        requires(std::derived_from<decltype(T::base), Base>)
    {
        std::cout << "scale_x: " << self().base.scale_x << std::endl;
        return self();
    }
    float& scale_x() const noexcept { return self().base.scale_x; }
    float& scale_y() const noexcept { return self().base.scale_y; }
    float& scale_z() const noexcept { return self().base.scale_z; }

    auto& set_scale(float x, float y, float z) const noexcept {
        self().base.scale_x = x;
        self().base.scale_y = y;
        self().base.scale_z = z;
        return self();
    }
};

template <typename T>
class PrintProperty {
    CRTP_CLASS(T)
   public:
    auto& print_property() const noexcept
        requires(std::derived_from<decltype(T::base), Base>)
    {
        std::cout << "pos_x: " << self().base.pos_x << std::endl;
        std::cout << "pos_y: " << self().base.pos_y << std::endl;
        std::cout << "pos_z: " << self().base.pos_z << std::endl;
        std::cout << "scale_x: " << self().base.scale_x << std::endl;
        std::cout << "scale_y: " << self().base.scale_y << std::endl;
        std::cout << "scale_z: " << self().base.scale_z << std::endl;
        return self();
    }
};

template <typename T, template <typename> class... Interfaces>
struct CompositeClass : public Interfaces<T>... {};
template <template <typename> class... Interfaces>
struct CompositeFrom : public Interfaces<CompositeFrom<Interfaces...>>... {};
template <typename T>
struct Test1 {
    CRTP_CLASS(T)
   public:
    Base base;
};
TEST(CompositeClassTest, TestAdd) {
    CompositeFrom<Test1, Scaleable, PrintProperty> test_class;
    test_class.set_scale(4.0f, 5.0f, 6.0f).print_property();
}
