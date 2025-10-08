#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

template <typename value_t, typename index_t, size_t bucket_size,
          value_t _InvalidValue = std::numeric_limits<value_t>::max(),
          index_t _InvalidIndex = std::numeric_limits<index_t>::max()>
class SparseList final {
    static_assert(bucket_size > 0, "");
    static constexpr index_t InvalidIndex = _InvalidIndex;
    static constexpr value_t InvalidValue = _InvalidValue;
    struct Bucket {
        using array_t = std::array<index_t, bucket_size>;
        std::unique_ptr<std::array<index_t, bucket_size>> data{nullptr};
        index_t& operator[](size_t i) {
            if (!data) {
                data = std::make_unique<array_t>();
                std::fill(data->begin(), data->end(), InvalidIndex);
            };
            return (*data)[i];
        }
        const index_t& operator[](size_t i) const {
            if (!data) return InvalidIndex;
            return (*data)[i];
        }
    };
    std::vector<value_t> values_;
    std::vector<Bucket> indices_buckets_;

   public:
    SparseList() = default;

    index_t insert(value_t value) {
        auto col = value / bucket_size;
        auto row = value % bucket_size;
        if (indices_buckets_.size() <= col) {
            indices_buckets_.resize(col + 1);
        }
        indices_buckets_[col][row] = values_.size();
        values_.push_back(value);
        return values_.size() - 1;
    }
    value_t get_value(index_t index) const { return values_[index]; }
    index_t get_index(value_t value) const {
        index_t col = value / bucket_size;
        index_t row = value % bucket_size;
        if (col >= indices_buckets_.size()) return InvalidIndex;
        return indices_buckets_[col][row];
    }
    void remove(index_t index) {
        if (index >= values_.size()) return;
        auto last_index = values_.size() - 1;
        if (last_index == index) {
            values_.pop_back();
            return;
        }
        auto col = values_[index] / bucket_size;
        auto row = values_[index] % bucket_size;
        auto last_col = values_[last_index] / bucket_size;
        auto last_row = values_[last_index] % bucket_size;
        values_[index] = values_[last_index];
        indices_buckets_[col][row] = InvalidIndex;
        indices_buckets_[last_col][last_row] = index;
        values_.pop_back();
    }
    bool has_value(value_t value) const {
        return get_index(value) != InvalidIndex;
    }
    bool has_index(index_t index) const { return index < values_.size(); }

    size_t bucket_count() const {
        return std::count_if(
            indices_buckets_.begin(), indices_buckets_.end(),
            [](auto&& bucket) { return bucket.data != nullptr; });
    }
    size_t size() const { return values_.size(); }

    auto begin() const { return values_.begin(); }
    auto end() const { return values_.end(); }
    auto cbegin() const { return values_.cbegin(); }
    auto cend() const { return values_.cend(); }
    auto rbegin() const { return values_.rbegin(); }
    auto rend() const { return values_.rend(); }
    auto crbegin() const { return values_.crbegin(); }
    auto crend() const { return values_.crend(); }
};