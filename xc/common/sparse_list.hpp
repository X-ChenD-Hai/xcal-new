#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
namespace xc::details {
template <typename T>
inline auto sparse_set_id_of_value(const T& value) {
    if constexpr (std::is_class_v<T>) {
        return value.id();
    } else {
        return value;
    }
}
template <typename T>
using id_of_t = decltype(sparse_set_id_of_value<T>(std::declval<T>()));
}  // namespace xc::details

template <typename value_t, typename index_t, size_t bucket_size,
          ::xc::details::id_of_t<value_t> _InvalidId =
              std::numeric_limits<::xc::details::id_of_t<value_t>>::max(),
          index_t _InvalidIndex = std::numeric_limits<index_t>::max()>
class SparseList final {
    static_assert(((bucket_size - 1) & bucket_size) == 0, "");
    using id_t = ::xc::details::id_of_t<value_t>;
    static constexpr index_t InvalidIndex = _InvalidIndex;
    static constexpr id_t InvalidId = _InvalidId;
    static constexpr id_t IdModMask = bucket_size - 1;
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

   protected:
    inline size_t bucket_idx(id_t id) const noexcept {
        return id / bucket_size;
    }
    inline size_t cell_idx(id_t id) const noexcept { return id & IdModMask; }
    inline std::tuple<size_t, size_t> bucket_cell_idx(
        value_t v) const noexcept {
        auto id = ::xc::details::sparse_set_id_of_value(v);
        return std::make_tuple(bucket_idx(id), cell_idx(id));
    }

   public:
    SparseList() = default;

    index_t insert(value_t value) {
        auto [col, row] = bucket_cell_idx(value);
        if (indices_buckets_.size() <= col) {
            indices_buckets_.resize(col + 1);
        } else if (indices_buckets_[col][row] != InvalidIndex) {
            values_[indices_buckets_[col][row]] = value;
            return indices_buckets_[col][row];
        }
        indices_buckets_[col][row] = values_.size();
        values_.push_back(value);
        return values_.size() - 1;
    }
    value_t get_value(index_t index) const { return values_[index]; }
    index_t get_index(value_t value) const {
        auto [col, row] = bucket_cell_idx(value);
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
        auto [col, row] = bucket_cell_idx(values_[index]);
        auto [last_col, last_row] = bucket_cell_idx(values_[last_index]);
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

   private:
    std::vector<value_t> values_;
    std::vector<Bucket> indices_buckets_;
};