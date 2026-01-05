#pragma once

#include <cstddef>
#include <cstdint>

#include "opengl_wrapper/types.hpp"

namespace xc::opengl {

void draw_arrays(DrawMode mode, int32_t first, size_t count);
void draw_elements(DrawMode mode, size_t count, const void* indices);

void draw_arrays_instanced(DrawMode mode, int32_t first, size_t count,
                           size_t instance_count);

void draw_elements_instanced(DrawMode mode, size_t count, const void* indices,
                             size_t instance_count);
void viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void clear_color(float red, float green, float blue, float alpha);
void clear(ClearBufferMaskFlag mask);
}  // namespace xc::opengl