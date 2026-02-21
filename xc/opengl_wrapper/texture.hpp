#pragma once

#include <cstdint>

#include "./types.hpp"
namespace xc::opengl {
using texture_id_t = uint32_t;

class Texture {
   public:
    Texture(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) = default;

   public:
    Texture(TextureTarget target);
    ~Texture();

    void bind(uint32_t unit) const noexcept;
    void unbind(uint32_t unit) const noexcept;

    void set_parameter(TextureWrap wrap_s, TextureWrap wrap_t,
                       TextureFilter min_filter,
                       TextureFilter mag_filter) const noexcept;

    void set_image_2d(int32_t level, TextureFormat internal_format,
                      int32_t width, int32_t height, TextureFormat format,
                      const void* data) const noexcept;

   private:
    texture_id_t id_;
    TextureTarget target_;
};

}  // namespace xc::opengl
