#pragma once
#include <cstdint>
#include <vector>

#include "./time_line.hpp"

namespace xc::xcal::animation {

class AnimationManager {
    friend void xc::xcal::animation::update_animation(
        AnimationManager &animation_manager, ecs::EventBus &event_bus);
    static constexpr uint32_t kInvalidTimeLineIndex =
        std::numeric_limits<uint32_t>::max();
    std::vector<AnimationInfo> animations_;
    std::vector<std::unique_ptr<TimeLine>> time_lines_;
    uint32_t current_time_line_index_{kInvalidTimeLineIndex};

   public:
    const TimeLine *current_time_line() const noexcept {
        if (current_time_line_index_ == kInvalidTimeLineIndex) {
            return nullptr;
        }
        return time_lines_[current_time_line_index_].get();
    }
    TimeLine *current_time_line() noexcept {
        return const_cast<TimeLine *>(std::as_const(*this).current_time_line());
    }
    TimeLine &create_time_line() {
        current_time_line_index_ = time_lines_.size();
        return *time_lines_.emplace_back(std::make_unique<TimeLine>());
    }
};
}  // namespace xc::xcal::animation