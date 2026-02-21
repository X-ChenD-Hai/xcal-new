#include <gtest/gtest.h>

#include <xc/common/id_generator.hpp>
#include <print>
namespace category {
class Component;
class Resource;
}  // namespace category

template <typename Component_>
using ComponentIdGenerator =
    ThreadSaftyIdGeneratorTemplate<category::Component,
                                   uint32_t>::Generator<Component_>;
using ComponentCounter =
    ThreadSaftyIdGeneratorTemplate<category::Component, uint32_t>;
template <typename Resource_>
using ResourceIdGenerator =
    ThreadSaftyIdGeneratorTemplate<category::Resource,
                                   uint32_t>::Generator<Resource_>;
using ResourceCounter =
    ThreadSaftyIdGeneratorTemplate<category::Resource, uint32_t>;

class Component1 {};
class Component2 {};
class Component3 {};
class Resource1 {};
class Resource2 {};
class Resource3 {};
TEST(IdGenerator, IdGenerator) {
    std::println("Component1 id: {}", ComponentIdGenerator<Component1>::get());
    std::println("Component1 id: {}", ComponentIdGenerator<Component2>::get());
    std::println("Component1 id: {}", ComponentIdGenerator<Component3>::get());
    std::println("Component1 count: {}", ComponentCounter::count());

    std::println("Resource1 id: {}", ResourceIdGenerator<Resource1>::get());
    std::println("Resource1 id: {}", ResourceIdGenerator<Resource2>::get());
    std::println("Resource1 id: {}", ResourceIdGenerator<Resource3>::get());
    std::println("Resource1 count: {}", ResourceCounter::count());
}