#pragma once

template <typename Derived>
class CrtpBase {
   protected:
    inline Derived& self() noexcept { return static_cast<Derived&>(*this); }
    inline const Derived& self() const noexcept {
        return static_cast<const Derived&>(*this);
    }
};