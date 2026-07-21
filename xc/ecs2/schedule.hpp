#pragma once
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "ecs2/entity.hpp"
#include "xc/ecs2/comman/conflict_matrix.hpp"
#include "xc/ecs2/comman/dependency_graph.hpp"
#include "xc/ecs2/comman/sparse_set.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/system.hpp"

namespace xc::ecs {

template <typename Sys, typename = void>
constexpr bool sys_has_name = false;
template <typename Sys>
constexpr bool
    sys_has_name<Sys, std::void_t<decltype(std::declval<Sys>().name())>> = true;

struct SystemInfo {
    size_t id;
    ComponentRegistry* registry;
    SparseSet<size_t> read_comp_list;
    SparseSet<size_t> write_comp_list;
    SparseSet<size_t> read_resource_list;
    SparseSet<size_t> write_resource_list;
    std::string name;

    template <typename Sys>
    static SystemInfo create(ComponentRegistry& registry, size_t id, Sys& s) {
        SystemInfo info{
            .id = id,
            .registry = &registry,
            .read_comp_list = {Sys::query_t::read_component_id_list(registry)},
            .write_comp_list = {Sys::query_t::write_component_id_list(
                registry)},
            .read_resource_list = {},
            .write_resource_list = {},
            .name = sys_name(s)};
        return info;
    }
    template <typename Sys>
    static std::string sys_name(Sys& s) {
        if constexpr (sys_has_name<Sys>) {
            return s.name();
        } else {
            return typeid(Sys).name();
        }
    }
};

class Schedule {
    friend struct std::formatter<xc::ecs::Schedule>;

   public:
    Schedule() = default;
    ~Schedule() = default;
    template <typename Sys, typename... Args>
    Schedule& add_system(Args&&... args) {
        systems_.emplace_back(
            (BaseSystem*)(new Sys(std::forward<Args>(args)...)));
        size_t id = systems_.size() - 1;
        system_infos_.emplace_back(SystemInfo::create<Sys>(
            *registry_, id, *(Sys*)(systems_.back().get())));
        conflict_matrix_.resize(conflict_matrix_.size() + 1);
        calculate_conflic(system_infos_.back());
        phases_derty_ = true;
        return *this;
    }
    ComponentRegistry& registry() { return *registry_; }
    const std::vector<SystemInfo>& system_infos() const {
        return system_infos_;
    }
    const ConflictMatrix& conflict_matrix() const { return conflict_matrix_; }
    const std::vector<std::vector<uint32_t>>& raw_phases() {
        if (phases_derty_) {
            flush_graphy();
            phases_ = graph_.phases();
            phases_derty_ = false;
        }
        return phases_;
    }
    void flush_graphy() {
        graph_.clear();
        graph_.resize(conflict_matrix_.size());
        for (uint32_t i = 0; i < conflict_matrix_.size(); ++i) {
            for (uint32_t j = 0; j < i; ++j) {
                if (conflict_matrix_.test(i, j)) {
                    graph_.add_edge(j, i);
                }
            }
        }
    }
    std::vector<std::vector<BaseSystem*>> phases() {
        std::vector<std::vector<BaseSystem*>> res;
        for (auto& p : raw_phases()) {
            res.emplace_back();
            for (auto i : p) {
                res.back().emplace_back(systems_[i].get());
            }
        }
        return res;
    }
    EntityFactory& entity_factory() { return entity_factory_; }

   protected:
    void calculate_conflic(const SystemInfo& info) {
        for (auto& o : system_infos_) {
            if (o.id == info.id) continue;
            for (auto r : o.read_comp_list) {
                if (info.write_comp_list.contains(r)) {
                    conflict_matrix_.set(o.id, info.id);
                }
            }
            for (auto w : o.write_comp_list) {
                if (info.read_comp_list.contains(w)) {
                    conflict_matrix_.set(info.id, o.id);
                }
                if (info.write_resource_list.contains(w)) {
                    conflict_matrix_.set(info.id, o.id);
                }
            }
        }
    }

   private:
    DependencyGraph graph_{};
    bool phases_derty_{true};
    std::vector<std::vector<uint32_t>> phases_{};
    ConflictMatrix conflict_matrix_{};
    std::vector<SystemInfo> system_infos_{};
    std::vector<std::unique_ptr<BaseSystem>> systems_{};
    std::unique_ptr<ComponentRegistry> registry_{new ComponentRegistry()};
    EntityFactory entity_factory_{};
};

}  // namespace xc::ecs

template <>
struct std::formatter<xc::ecs::SystemInfo> {
    constexpr auto parse(format_parse_context& ctx) { return ++ctx.begin(); }
    auto format(const xc::ecs::SystemInfo& info,
                std::format_context& ctx) const {
        return std::format_to(ctx.out(),
                              "SystemInfo(id={}, registry={}, "
                              "read_comp_list={}, write_comp_list={}, "
                              "read_resource_list={}, write_resource_list={})",
                              info.id, (void*)info.registry,
                              info.read_comp_list, info.write_comp_list,
                              info.read_resource_list,
                              info.write_resource_list);
    }
};
template <>
struct std::formatter<xc::ecs::Schedule> {
    constexpr auto parse(format_parse_context& ctx) { return ++ctx.begin(); }
    auto format(const xc::ecs::Schedule& schedule,
                std::format_context& ctx) const {
        return std::format_to(
            ctx.out(), "Schedule(system_infos={}, systems({}), registry={})",
            schedule.system_infos_, schedule.systems_.size(),
            *schedule.registry_);
    }
};
