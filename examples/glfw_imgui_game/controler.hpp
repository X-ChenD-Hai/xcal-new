#pragma once
#include <ecs/types.hpp>

#include "ecs/world.hpp"

class Controler {
    friend class ::ecs::World;

   public:
    Controler(const Controler&) = delete;
    Controler(Controler&&) = delete;
    Controler& operator=(const Controler&) = delete;
    Controler& operator=(Controler&&) = delete;

   protected:
    Controler() {}

    static Controler* install(ecs::World& world) { return new Controler{}; }
    void update() {}

   public:
   private:
};