#include "./texture.hpp"

#include "./opengl_api.hpp"

using namespace xc::opengl;

Texture::Texture(TextureTarget target) : id_{0}, target_{target} {
    glGenTextures(1, &id_);
}

Texture::~Texture() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

void Texture::bind(uint32_t unit) const noexcept {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(enum_to_gl(target_), id_);
}

void Texture::unbind(uint32_t unit) const noexcept {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(enum_to_gl(target_), 0);
}

void Texture::set_parameter(TextureWrap wrap_s, TextureWrap wrap_t,
                            TextureFilter min_filter,
                            TextureFilter mag_filter) const noexcept {
    glTexParameteri(enum_to_gl(target_), GL_TEXTURE_WRAP_S, enum_to_gl(wrap_s));
    glTexParameteri(enum_to_gl(target_), GL_TEXTURE_WRAP_T, enum_to_gl(wrap_t));
    glTexParameteri(enum_to_gl(target_), GL_TEXTURE_MIN_FILTER,
                    enum_to_gl(min_filter));
    glTexParameteri(enum_to_gl(target_), GL_TEXTURE_MAG_FILTER,
                    enum_to_gl(mag_filter));
}

void Texture::set_image_2d(int32_t level, TextureFormat internal_format,
                           int32_t width, int32_t height, TextureFormat format,
                           const void* data) const noexcept {
    glTexImage2D(enum_to_gl(target_), level, enum_to_gl(internal_format), width,
                 height, 0, enum_to_gl(format), GL_UNSIGNED_BYTE, data);
}
