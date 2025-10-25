#pragma once
#include <chrono>
#include <cstdint>
#include <variant>
#include <vector>

#include "./animation.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"

namespace xc::xcal::animation {
void update_animation(AnimationManager &animation_manager,
                      ecs::EventBus &event_bus);
struct AnimationInfo {
    float start_time;
    float end_time;
    ecs::Entity entity;
    ecs::component_t component_id;
    std::variant<float *, double *> component_field_pointer;
};

class TimeLine {
    friend void update_animation(AnimationManager &animation_manager,
                                 ecs::EventBus &event_bus);

   private:
    std::vector<Animation> animations_;
    bool is_playing_{false};
    std::chrono::steady_clock::time_point stop_time_;
    std::chrono::steady_clock::time_point last_update_time_;

   protected:
    [[nodiscard]] inline float update() {
        if (!is_playing_) return 0.0f;
        auto delta_time = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - last_update_time_);
        last_update_time_ = std::chrono::steady_clock::now();
        return delta_time.count();
    }

   public:
    TimeLine() = default;
    inline void add_animation(Animation animation) {
        animations_.push_back(animation);
    }
    inline void play() {
        is_playing_ = true;
        last_update_time_ = std::chrono::steady_clock::now();
    }

    inline void stop() {
        is_playing_ = false;
        stop_time_ = std::chrono::steady_clock::now();
    }
    inline bool is_playing() const { return is_playing_; }
};
}  // namespace xc::xcal::animation
