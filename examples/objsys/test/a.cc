#include <objsys.hpp>
#include <print>
#include <ranges>

enum class Type {
    A,
    B,
    C,
};
using MySystem = EnumObjectMap<Type, (size_t)Type::A, (size_t)Type::C + 1>;

class A2 : public EnumBase<A2, Type::A, MySystem> {
   public:
    A2() { std::println("A2 object created!"); }
    ~A2() { std::println("A2 object destroyed!"); }
};
template const bool A2::BaseClass::reg;

class B2 : public EnumBase<B2, Type::B, MySystem> {
   public:
    B2() { std::println("B2 object created!"); }
    ~B2() { std::println("B2 object destroyed!"); }
};
template const bool B2::BaseClass::reg;

class C2 : public EnumBase<C2, Type::C, MySystem> {
   public:
    C2() { std::println("C2 object created!"); }
    ~C2() { std::println("C2 object destroyed!"); }
};
template const bool C2::BaseClass::reg;

class DA {};
class DB {};
class DC {};

template<class T>
void add(T *t){
    MySystem::instence().call_object(*t);
}

int main() {
    std::println("__________________");
    A2::BaseClass *a2 = new A2();
    a2->call();
    delete (A2 *)a2;
    B2::BaseClass *b2 = new B2();
    b2->call();
    delete (B2 *)b2;
    C2::BaseClass *c2 = new C2();
    c2->call();
    delete (C2 *)c2;
    auto v =
        std::ranges::views::transform([](const Type &k) { return (size_t)k; });
    std::println("{}", MySystem::instence().class_names());
    std::println("{}", MySystem::instence().class_keys() | v);

    return 0;
}