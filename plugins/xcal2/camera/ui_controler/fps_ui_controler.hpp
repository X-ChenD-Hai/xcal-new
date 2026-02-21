#pragma once
#include <xc/ecs/types.hpp>
#include <xc/ecs/world.hpp>

#include "../fps_camera_controler.hpp"


namespace xcal::camera::ui_controler {
class FPSUIControler {
    FPSUIControler(const FPSUIControler&) = default;
    FPSUIControler(FPSUIControler&&) = default;
    FPSUIControler& operator=(const FPSUIControler&) = default;
    FPSUIControler& operator=(FPSUIControler&&) = default;
    friend class ::ecs::World;

   public:
    inline double dx() const noexcept { return dx_; }
    inline double dy() const noexcept { return dy_; }
    inline double dz() const noexcept { return dz_; }
    inline double dyaw() const noexcept { return dyaw_; }
    inline double dpitch() const noexcept { return dpitch_; }
    inline double dzoom() const noexcept { return dzoom_; }

    inline FPSUIControler& set_dx(double dx) noexcept {
        dx_ = dx;
        return *this;
    }
    inline FPSUIControler& set_dy(double dy) noexcept {
        dy_ = dy;
        return *this;
    }
    inline FPSUIControler& set_dz(double dz) noexcept {
        dz_ = dz;
        return *this;
    }
    inline FPSUIControler& set_dyaw(double dyaw) noexcept {
        dyaw_ = dyaw;
        return *this;
    }
    inline FPSUIControler& set_dpitch(double dpitch) noexcept {
        dpitch_ = dpitch;
        return *this;
    }
    inline FPSUIControler& set_dzoom(double dzoom) noexcept {
        dzoom_ = dzoom;
        return *this;
    }

    inline FPSUIControler& set_dtranslation(double dx, double dy,
                                            double dz) noexcept {
        dx_ = dx;
        dy_ = dy;
        dz_ = dz;
        return *this;
    }
    inline FPSUIControler& set_drotation(double dyaw, double dpitch) noexcept {
        dyaw_ = dyaw;
        dpitch_ = dpitch;
        return *this;
    }

   protected:
    FPSUIControler() = default;
    static FPSUIControler* install(ecs::World& world) {
        return new FPSUIControler();
    }
    void run(ecs::EventBus& event_bus,
             xcal::camera::FpsCameraControler& camera_controler);
    static inline void uninstall(ecs::World& world, FPSUIControler* ptr) {
        delete ptr;
    }

   private:
    double dx_{1}, dy_{1}, dz_{1};
    double dyaw_{1}, dpitch_{1};
    double dzoom_{1};
};

}  // namespace xcal::camera::ui_controler