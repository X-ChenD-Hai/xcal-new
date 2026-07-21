#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <vector>
namespace xc::ecs {
class DependencyGraph {
    friend struct std::formatter<DependencyGraph>;

   public:
    DependencyGraph() = default;
    DependencyGraph(uint32_t node_count)
        : successors_(node_count), in_degree_(node_count, 0) {}

    void clear() {
        successors_.clear();
        in_degree_.clear();
    }

    void resize(uint32_t size) {
        assert(size > node_count());
        successors_.resize(size);
        in_degree_.resize(size, 0);
    }

    void add_edge(uint32_t from, uint32_t to) {
        auto s = std::max(from, to);
        assert(s < node_count());
        successors_[from].push_back(to);
        ++in_degree_[to];
    }
    const std::vector<uint32_t>& successors(uint32_t node) const {
        assert(node < node_count());
        return successors_[node];
    }
    uint32_t node_count() const { return successors_.size(); }
    uint32_t in_degree(uint32_t node) const {
        assert(node < node_count());
        return in_degree_[node];
    }
    uint32_t& in_degree(uint32_t node) {
        assert(node < node_count());
        return in_degree_[node];
    }
    std::vector<std::vector<uint32_t>> phases() const {
        std::vector<uint32_t> in_degree = in_degree_;
        std::vector<std::vector<uint32_t>> phases{};
        std::vector<uint32_t> ready{};
        std::vector<uint32_t> successor{};

        for (auto i = 0u; i < node_count(); ++i) {
            if (in_degree[i] == 0) {
                successor.push_back(i);
            }
        }

        while (!successor.empty()) {
            auto tmp = std::move(successor);
            successor.clear();
            for (auto i : tmp) {
                if (in_degree[i] != 0) continue;
                ready.push_back(i);
                for (auto s : successors(i)) {
                    if (--in_degree[s] != 0) continue;
                    successor.push_back(s);
                }
            }
            phases.emplace_back(std::move(ready));
            ready.clear();
        }
        return phases;
    }

   private:
    std::vector<std::vector<uint32_t>> successors_{};
    std::vector<uint32_t> in_degree_{};
};
}  // namespace xc::ecs
template <>
struct std::formatter<xc::ecs::DependencyGraph> {
    constexpr auto parse(format_parse_context& ctx) { return ++ctx.begin(); }
    auto format(const xc::ecs::DependencyGraph& e, format_context& ctx) const
        -> decltype(ctx.out()) {
        return std::format_to(ctx.out(), "DependencyGraph({})",
                              e.successors_.size());
    }
};