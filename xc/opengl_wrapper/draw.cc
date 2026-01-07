#include "draw.hpp"

#include "opengl_wrapper/opengl_api.hpp"

void xc::opengl::draw_arrays(xc::opengl::DrawMode mode, int32_t first,
                             size_t count) {
    glDrawArrays(enum_to_gl(mode), first, count);
}

void xc::opengl::draw_elements(xc::opengl::DrawMode mode, size_t count,
                               const void* indices) {
    glDrawElements(enum_to_gl(mode), count, enum_to_gl(DataType::UNSIGNED_INT),
                   indices);
}
void xc::opengl::viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    glViewport(x, y, width, height);
}
void xc::opengl::clear_color(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
}
void xc::opengl::clear(ClearBufferMaskFlag mask) { glClear(enum_to_gl(mask)); }