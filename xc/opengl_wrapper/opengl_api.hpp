#pragma once

// OpenGL后端宏定义，与现有openglloader.h保持一致
#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include "glbinding/gl/bitfield.h"
#include "glbinding/gl/enum.h"
using namespace ::gl;
#elif defined(USE_GLAD)
#include <glad/gl.h>
#define GL_NONE_BIT 0x0
#define eeeeeeeeeeeeeeee
#else
#error \
    "No OpenGL backend defined. Please define either USE_GLBINDING or USE_GLAD"
#endif
#include "./types.hpp"
namespace xc::opengl {

template <typename T>
constexpr GLenum enum_to_gl(T value) {
    static_assert(false, "enum_to_gl not implemented for this type");
}

template <>
inline constexpr GLenum enum_to_gl(BufferTarget value) {
    switch (value) {
        case BufferTarget::ARRAY:
            return GL_ARRAY_BUFFER;
        case BufferTarget::ELEMENT_ARRAY:
            return GL_ELEMENT_ARRAY_BUFFER;
        case BufferTarget::UNIFORM:
            return GL_UNIFORM_BUFFER;
        case BufferTarget::SHADER_STORAGE:
            return GL_SHADER_STORAGE_BUFFER;
        case BufferTarget::DRAW_INDIRECT:
            return GL_DRAW_INDIRECT_BUFFER;
        case BufferTarget::COPY_READ:
            return GL_COPY_READ_BUFFER;
        case BufferTarget::COPY_WRITE:
            return GL_COPY_WRITE_BUFFER;
        default:
            return GL_INVALID_ENUM;
    }
}
template <>
inline constexpr GLenum enum_to_gl(BufferUsage value) {
    switch (value) {
        case BufferUsage::STATIC_DRAW:
            return GL_STATIC_DRAW;
        case BufferUsage::DYNAMIC_DRAW:
            return GL_DYNAMIC_DRAW;
        case BufferUsage::STREAM_DRAW:
            return GL_STREAM_DRAW;
        default:
            return GL_INVALID_ENUM;
    }
}
template <>
inline constexpr GLenum enum_to_gl(ShaderSourceType value) {
    switch (value) {
        case ShaderSourceType::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderSourceType::Fragment:
            return GL_FRAGMENT_SHADER;
        case ShaderSourceType::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderSourceType::Compute:
            return GL_COMPUTE_SHADER;
        default:
            return GL_INVALID_ENUM;
    }
}
template <>
inline constexpr GLenum enum_to_gl(TextureTarget value) {
    switch (value) {
        case TextureTarget::TEXTURE_1D:
            return GL_TEXTURE_1D;
        case TextureTarget::TEXTURE_2D:
            return GL_TEXTURE_2D;
        case TextureTarget::TEXTURE_3D:
            return GL_TEXTURE_3D;
        case TextureTarget::TEXTURE_1D_ARRAY:
            return GL_TEXTURE_1D_ARRAY;
        case TextureTarget::TEXTURE_2D_ARRAY:
            return GL_TEXTURE_2D_ARRAY;
        case TextureTarget::TEXTURE_RECTANGLE:
            return GL_TEXTURE_RECTANGLE;
        case TextureTarget::TEXTURE_CUBE_MAP:
            return GL_TEXTURE_CUBE_MAP;
        case TextureTarget::TEXTURE_CUBE_MAP_ARRAY:
            return GL_TEXTURE_CUBE_MAP_ARRAY;
        case TextureTarget::TEXTURE_BUFFER:
            return GL_TEXTURE_BUFFER;
        case TextureTarget::TEXTURE_2D_MULTISAMPLE:
            return GL_TEXTURE_2D_MULTISAMPLE;
        case TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY:
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        default:
            return GL_INVALID_ENUM;
    }
}

template <>
inline constexpr GLenum enum_to_gl(TextureFormat value) {
    switch (value) {
        case TextureFormat::RED:
            return GL_RED;
        case TextureFormat::RG:
            return GL_RG;
        case TextureFormat::RGB:
            return GL_RGB;
        case TextureFormat::BGR:
            return GL_BGR;
        case TextureFormat::RGBA:
            return GL_RGBA;
        case TextureFormat::BGRA:
            return GL_BGRA;
        case TextureFormat::DEPTH_COMPONENT:
            return GL_DEPTH_COMPONENT;
        case TextureFormat::DEPTH_STENCIL:
            return GL_DEPTH_STENCIL;
        default:
            return GL_INVALID_ENUM;
    }
}

template <>
inline constexpr GLenum enum_to_gl(TextureWrap value) {
    switch (value) {
        case TextureWrap::REPEAT:
            return GL_REPEAT;
        case TextureWrap::MIRRORED_REPEAT:
            return GL_MIRRORED_REPEAT;
        case TextureWrap::CLAMP_TO_EDGE:
            return GL_CLAMP_TO_EDGE;
        case TextureWrap::CLAMP_TO_BORDER:
            return GL_CLAMP_TO_BORDER;
        default:
            return GL_INVALID_ENUM;
    }
}

template <>
inline constexpr GLenum enum_to_gl(TextureFilter value) {
    switch (value) {
        case TextureFilter::NEAREST:
            return GL_NEAREST;
        case TextureFilter::LINEAR:
            return GL_LINEAR;
        case TextureFilter::NEAREST_MIPMAP_NEAREST:
            return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilter::LINEAR_MIPMAP_NEAREST:
            return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilter::NEAREST_MIPMAP_LINEAR:
            return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilter::LINEAR_MIPMAP_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            return GL_INVALID_ENUM;
    }
}

template <>
inline constexpr GLenum enum_to_gl(DrawMode value) {
    switch (value) {
        case DrawMode::POINTS:
            return GL_POINTS;
        case DrawMode::LINES:
            return GL_LINES;
        case DrawMode::LINE_LOOP:
            return GL_LINE_LOOP;
        case DrawMode::LINE_STRIP:
            return GL_LINE_STRIP;
        case DrawMode::TRIANGLES:
            return GL_TRIANGLES;
        case DrawMode::TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
        case DrawMode::TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case DrawMode::LINES_ADJACENCY:
            return GL_LINES_ADJACENCY;
        case DrawMode::LINE_STRIP_ADJACENCY:
            return GL_LINE_STRIP_ADJACENCY;
        case DrawMode::TRIANGLES_ADJACENCY:
            return GL_TRIANGLES_ADJACENCY;
        case DrawMode::TRIANGLE_STRIP_ADJACENCY:
            return GL_TRIANGLE_STRIP_ADJACENCY;
        case DrawMode::POLYGON:
            return GL_POLYGON;
        default:
            return GL_INVALID_ENUM;
    }
}
template <>
inline constexpr GLenum enum_to_gl<DataType>(DataType type) {
    switch (type) {
        case DataType::BYTE:
            return GL_BYTE;
        case DataType::UNSIGNED_BYTE:
            return GL_UNSIGNED_BYTE;
        case DataType::SHORT:
            return GL_SHORT;
        case DataType::UNSIGNED_SHORT:
            return GL_UNSIGNED_SHORT;
        case DataType::INT:
            return GL_INT;
        case DataType::UNSIGNED_INT:
            return GL_UNSIGNED_INT;
        case DataType::FLOAT:
            return GL_FLOAT;
        case DataType::DOUBLE:
            return GL_DOUBLE;
        default:
            return GL_INVALID_ENUM;
    }
}

inline constexpr auto enum_to_gl(ClearBufferMask value) {
    switch (value) {
        case ClearBufferMask::COLOR_BUFFER_BIT:
            return GL_COLOR_BUFFER_BIT;
        case ClearBufferMask::DEPTH_BUFFER_BIT:
            return GL_DEPTH_BUFFER_BIT;
        case ClearBufferMask::STENCIL_BUFFER_BIT:
            return GL_STENCIL_BUFFER_BIT;
    }
}

inline constexpr auto enum_to_gl(Flag<ClearBufferMask> value) {
    auto mask = GL_NONE_BIT;
    if (value.test(ClearBufferMask::COLOR_BUFFER_BIT)) {
        mask |= enum_to_gl(ClearBufferMask::COLOR_BUFFER_BIT);
    }
    if (value.test(ClearBufferMask::DEPTH_BUFFER_BIT)) {
        mask |= enum_to_gl(ClearBufferMask::DEPTH_BUFFER_BIT);
    }
    if (value.test(ClearBufferMask::STENCIL_BUFFER_BIT)) {
        mask |= enum_to_gl(ClearBufferMask::STENCIL_BUFFER_BIT);
    }
    return mask;
}

}  // namespace xc::opengl