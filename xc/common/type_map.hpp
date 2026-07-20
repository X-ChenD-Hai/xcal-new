#pragma once
#include <atomic>
#include <cstddef>
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
template <typename T, typename FillValue = T>
class TypeMap {
    static inline std::atomic_size_t next_type_id{0};
    const FillValue fill_value_{};
    const T invalid_value_{};
    std::vector<T> table_{};

   private:
    template <typename Tp>
    static const size_t type_id_;

   public:
    TypeMap(const FillValue& fill_value = -1)
        : fill_value_(fill_value), invalid_value_((T)fill_value), table_{} {}
    template <typename Tp>
    size_t type_id() const {
        return type_id_<Tp>;
    }
    template <typename Tp>
    T& data() {
        return data(type_id_<Tp>);
    }
    template <typename Tp>
    const T& data() const {
        return data(type_id_<Tp>);
    }
    const T& data(size_t idx) const {
        if (table_.size() <= idx) return invalid_value_;
        return table_[idx];
    }
    T& data(size_t idx) {
        if (table_.size() <= idx) {
            while (table_.size() <= idx) {
                table_.emplace_back((T)fill_value_);
            }
        }
        return table_[idx];
    }

    std::vector<T>::iterator begin() { return table_.begin(); }
    std::vector<T>::iterator end() { return table_.end(); }
    std::vector<T>::const_iterator begin() const { return table_.begin(); }
    std::vector<T>::const_iterator end() const { return table_.end(); }
};

template <typename T, typename F>
template <typename Tp>
const size_t TypeMap<T, F>::type_id_ = TypeMap<T, F>::next_type_id++;
template <typename T>
TypeMap(const T&) -> TypeMap<T, T>;