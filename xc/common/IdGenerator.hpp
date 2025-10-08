#pragma once
#include <atomic>
template <typename Category, typename index_t>
class IdGeneratorTemplate final {
    inline static index_t next_id_{1};

   public:
    static auto count() { return next_id_ - 1; }
    template <class Class>
    struct Generator {
       public:
        static auto get() {
            static auto id = next_id_++;
            return id;
        }
    };
};
template <typename Category, typename index_t>
using ThreadSaftyIdGeneratorTemplate =
    IdGeneratorTemplate<Category, std::atomic<index_t>>;