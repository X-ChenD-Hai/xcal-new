
#include <vcruntime_typeinfo.h>

#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T, typename... U>
constexpr size_t component_id = std::numeric_limits<size_t>::max();
template <typename T, typename V, typename... U>
constexpr size_t component_id<T, V, U...> = component_id<T, U...> + 1;
template <typename T, typename... U>
constexpr size_t component_id<T, T, U...> = 0;
template <typename T>
constexpr size_t component_id<T> = 1;

template <typename... T>
class Word;

struct ComponentInfo {
    template <typename T, size_t idx>
    static constexpr ComponentInfo make() {
        return ComponentInfo{
            [](void* p) {
                if constexpr (!std::is_trivially_destructible_v<T>)
                    std::destroy_at((T*)p);
            },
            idx, typeid(T).name()};
    }

    void (*const distractor)(void* ptr);
    const size_t id;
    std::string_view name;
};
template <typename T>
class Pool {
   public:
    std::vector<T> pool;
    std::unordered_map<size_t, size_t> map;
};

template <typename... T>
class ComponentTable {
   public:
    constexpr ComponentTable()
        : info_{[]<size_t... I>(std::index_sequence<I...>) {
              return decltype(info_){ComponentInfo::make<T, I>()...};
          }(std::make_index_sequence<sizeof...(T)>())} {}
    ~ComponentTable() {}

   public:
    template <typename U>
    static constexpr size_t id() {
        return component_id<U, T...>;
    }
    template <typename U>
    static constexpr bool has() {
        return id<U>() < sizeof...(T);
    }
    template <typename U>
    constexpr const ComponentInfo& info() {
        return std::get<id<U>()>(info_);
    }

   private:
    const std::array<ComponentInfo, sizeof...(T)> info_{};
    std::tuple<std::vector<T>...> pools_;
};
template <typename Local, typename Global>
class WordBuilder {
   public:
    WordBuilder() {}
    ~WordBuilder() {}
};

int main(int argc, char* argv[]) {
    ComponentTable<int, float, size_t> tbl;

    return 0;
}