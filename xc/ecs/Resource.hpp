#pragma once
#include <memory>
#include "./utils/traits.hpp"
using Cell = std::unique_ptr<void, std::function<void(void *)>>;

class ResourceIdGenerator {
    static inline size_t next_id_{0};

   public:
    template <typename T>
    static size_t get() {
        static size_t id = _get<purge_t<T>>();
        return id;
    }

   private:
    template <typename T>
    static size_t _get() {
        static size_t id = next_id_++;
        return id;
    }
};

struct ResourceInfo {
    size_t id;
    Cell resource_;
};