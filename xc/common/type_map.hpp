#pragma once
#include <vector>

class TypeIdGenerator {
    static inline size_t next_type_id{0};
    std::vector<size_t> table_;
    size_t cur_id_ = 0;

   public:
    template <typename T>
    size_t type_id() {
        static const size_t tid = next_type_id++;
        if (table_.size() <= tid) {
            table_.resize(tid + 1, -1);
            return table_[tid] = cur_id_++;
        } else if (table_[tid] != -1) {
            return table_[tid];
        } else {
            return table_[tid] = cur_id_++;
        }
    }
};
template <typename T>
class TypeMap {
    static inline size_t next_type_id{0};
    const T fill_value_{};
    std::vector<T> table_{};

   private:
    template <typename Tp>
    static const size_t type_id;

   public:
    TypeMap(const T& fill_value = -1) : fill_value_(fill_value), table_{} {}
    template <typename Tp>
    T& data() {
        if (table_.size() <= type_id<Tp>)
            table_.resize(type_id<Tp> + 1, fill_value_);
        return table_[type_id<Tp>];
    }
    template <typename Tp>
    const T& data() const {
        if (table_.size() <= type_id<Tp>) return fill_value_;
        return table_[type_id<Tp>];
    }
    std::vector<T>::iterator begin() { return table_.begin(); }
    std::vector<T>::iterator end() { return table_.end(); }
    std::vector<T>::const_iterator begin() const { return table_.begin(); }
    std::vector<T>::const_iterator end() const { return table_.end(); }
};

template <typename T>
template <typename Tp>
const size_t TypeMap<T>::type_id = TypeMap<T>::next_type_id++;