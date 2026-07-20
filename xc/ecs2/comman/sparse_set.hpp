#include <format>
#include <vector>
namespace ecs {
template <typename T>
struct SparseSetValueInfo;
template <typename T>
    requires(std::is_integral_v<T>)
struct SparseSetValueInfo<T> {
    using id_t = T;
    using version_t = T;
    static id_t id_of(T value) { return value; }
    static version_t version_of(T value) { return 0; }
    static constexpr id_t InvalidId = std::numeric_limits<id_t>::max();
};
template <typename T>
class SparseSet {
   public:
    using info = SparseSetValueInfo<T>;
    using id_t = info::id_t;
    using version_t = info::version_t;
    static constexpr id_t InvalId = info::InvalidId;
    SparseSet() = default;
    static id_t id_of(const T& value) { return info::id_of(value); }
    static version_t version_of(const T& value) {
        return info::version_of(value);
    }
    void insert(const T& value) {
        assert(!contains(value));
        auto id = id_of(value);
        if (n >= dense_.size()) {
            dense_.resize(n + 1, value);
        } else {
            dense_[n] = value;
        }
        if (sparse_.size() <= id) {
            sparse_.resize(id + 1, InvalId);
        }
        sparse_[id] = n;
        ++n;
    }
    bool contains(const T& value) const {
        if (id_of(value) >= sparse_.size()) return false;
        auto idx = sparse_[id_of(value)];
        return idx < n && version_of(dense_[idx]) == version_of(value);
    }
    T& operator[](id_t id) {
        assert(id < sparse_.size());
        assert(sparse_[id] < n);
        return value_of(sparse_[id]);
    }
    const T& operator[](id_t id) const {
        assert(id < sparse_.size());
        assert(sparse_[id] < n);
        return value_of(sparse_[id]);
    }
    size_t size() const noexcept { return n; }
    bool empty() const noexcept { return n == 0; }
    void clear() { n = 0; }
    id_t& index_of(const T& value) { return sparse_[id_of(value)]; }
    id_t index_of(const T& value) const { return sparse_[id_of(value)]; }
    T& value_of(const id_t& id) { return dense_[id]; }
    const T& value_of(const id_t& id) const { return dense_[id]; }
    void erase(const T& value) {
        assert(contains(value));
        auto idx = index_of(value);
        auto& last_val = value_of(n - 1);
        value_of(idx) = last_val;
        index_of(last_val) = idx;
        --n;
    }
    std::string to_string() const {
        std::string sparse_str = "[";
        for (auto s : sparse_) {
            sparse_str += s != InvalId ? std::format("{},", s) : "_,";
        }
        sparse_str.pop_back();
        sparse_str += "]";
        std::string dense_str = "[";
        for (auto d : dense_) {
            dense_str += std::format("{}#{},", id_of(d), version_of(d));
        }
        dense_str.pop_back();
        dense_str += "]";

        return std::format("dense = {}, sparse = {}, n = {}", dense_str,
                           sparse_str, n);
    }

    decltype(auto) begin() { return dense_.begin(); }
    decltype(auto) end() { return dense_.begin() + n; }
    decltype(auto) cbegin() const { return dense_.cbegin(); }
    decltype(auto) cend() const { return dense_.begin() + n; }
    decltype(auto) rbegin() { return dense_.rend() - n; }
    decltype(auto) rend() { return dense_.rend(); }
    decltype(auto) crbegin() const { return dense_.rend() - n; }
    decltype(auto) crend() const { return dense_.crend(); }

    std::vector<T> dense_{};
    std::vector<id_t> sparse_{};
    size_t n{0};
};
}  // namespace ecs