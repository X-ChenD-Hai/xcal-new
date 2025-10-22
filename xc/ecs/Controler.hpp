#pragma once
#include "./world.hpp"

namespace ecs {
class Controler {
    std::unique_ptr<World> world_;
    std::vector<std::function<void(World &world)>> systems_;

   public:
    Controler() = default;
    ~Controler() = default;

   public:
    Controler(Controler &&) = delete;
    Controler &operator=(Controler &&) = delete;
    Controler(const Controler &) = delete;
    Controler &operator=(const Controler &) = delete;
};
}  // namespace ecs
