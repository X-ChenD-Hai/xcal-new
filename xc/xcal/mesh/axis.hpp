#pragma once

class Axis {
   private:
    double x_, y_, z_;

   public:
    Axis(double x, double y, double z) : x_(x), y_(y), z_(z) {}
    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }
};