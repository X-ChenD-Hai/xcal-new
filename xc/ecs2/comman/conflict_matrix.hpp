#include <cstddef>
#include <format>
#include <vector>

namespace xc::ecs {
class ConflictMatrix {
    friend struct std::formatter<xc::ecs::ConflictMatrix>;

   public:
    ConflictMatrix() = default;
    ConflictMatrix(size_t num_sys) {
        matrix_.resize(num_sys, std::vector<bool>(num_sys, false));
    }
    bool test(size_t row, size_t col) const { return matrix_[row][col]; }
    void set(size_t row, size_t col) { matrix_[row][col] = true; }
    void unset(size_t row, size_t col) { matrix_[row][col] = false; }
    size_t size() const { return matrix_.size(); }
    void resize(size_t size) {
        matrix_.resize(size, std::vector<bool>(size, false));
        for (auto& row : matrix_) {
            row.resize(size, false);
        }
    }

   private:
    std::vector<std::vector<bool>> matrix_;
};

}  // namespace xc::ecs

template <>
struct std::formatter<xc::ecs::ConflictMatrix> {
    constexpr auto parse(format_parse_context& ctx) { return ++ctx.begin(); }
    auto format(const xc::ecs::ConflictMatrix& matrix,
                std::format_context& ctx) const {
        auto o = ctx.out();
        for (size_t i = 0; i < matrix.matrix_.size(); ++i) {
            *o++ = '\n';
            for (size_t j = 0; j < matrix.matrix_.size(); ++j) {
                *o++ = matrix.test(i, j) ? '1' : '0';
                if (j < matrix.matrix_.size() - 1) *o++ = ' ';
            }
        }
        *o++ = '\n';
        return o;
    }
};
