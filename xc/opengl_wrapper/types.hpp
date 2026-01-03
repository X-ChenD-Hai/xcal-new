#pragma once

#include <cstdint>
#include <type_traits>
namespace xc::opengl {
using buffer_id_t = uint32_t;
enum class BufferTarget {
    // 顶点属性数据（GL_ARRAY_BUFFER）
    ARRAY,
    // 顶点索引数据（GL_ELEMENT_ARRAY_BUFFER）
    ELEMENT_ARRAY,
    // 统一变量缓冲（GL_UNIFORM_BUFFER）
    UNIFORM,
    // 着色器存储缓冲（GL_SHADER_STORAGE_BUFFER）
    SHADER_STORAGE,
    // 间接绘制命令（GL_DRAW_INDIRECT_BUFFER）
    DRAW_INDIRECT,
    // 缓冲期数据读取（GL_COPY_READ_BUFFER）
    COPY_READ,
    // 缓冲期数据写入（GL_COPY_WRITE_BUFFER）
    COPY_WRITE
};
enum class BufferUsage {
    // 数据修改一次，使用几次（GL_STREAM_DRAW）
    STREAM_DRAW,
    // 数据修改一次，使用多次（GL_STATIC_DRAW）
    STATIC_DRAW,
    // 数据修改多次，使用多次（GL_DYNAMIC_DRAW）
    DYNAMIC_DRAW,
    // 数据从OpenGL读取一次，使用几次（GL_STREAM_READ）
    STREAM_READ,
    // 数据从OpenGL读取一次，使用多次（GL_STATIC_READ）
    STATIC_READ,
    // 数据从OpenGL读取多次，使用多次（GL_DYNAMIC_READ）
    DYNAMIC_READ,
    // 数据从OpenGL拷贝一次，使用几次（GL_STREAM_COPY）
    STREAM_COPY,
    // 数据从OpenGL拷贝一次，使用多次（GL_STATIC_COPY）
    STATIC_COPY,
    // 数据从OpenGL拷贝多次，使用多次（GL_DYNAMIC_COPY）
    DYNAMIC_COPY
};
using vertex_array_id_t = uint32_t;

enum class DataType {
    BYTE,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    DOUBLE
};
enum class DrawMode {
    // 点（GL_POINTS）
    POINTS,
    // 线段（GL_LINES）
    LINES,
    // 线环（GL_LINE_LOOP）
    LINE_LOOP,
    // 线带（GL_LINE_STRIP）
    LINE_STRIP,
    // 三角形（GL_TRIANGLES）
    TRIANGLES,
    // 三角形带（GL_TRIANGLE_STRIP）
    TRIANGLE_STRIP,
    // 三角形扇（GL_TRIANGLE_FAN）
    TRIANGLE_FAN,
    // 邻接线段（GL_LINES_ADJACENCY，OpenGL 3.2+）
    LINES_ADJACENCY,
    // 邻接线带（GL_LINE_STRIP_ADJACENCY，OpenGL 3.2+）
    LINE_STRIP_ADJACENCY,
    // 邻接三角形（GL_TRIANGLES_ADJACENCY，OpenGL 3.2+）
    TRIANGLES_ADJACENCY,
    // 邻接三角形带（GL_TRIANGLE_STRIP_ADJACENCY，OpenGL 3.2+）
    TRIANGLE_STRIP_ADJACENCY
};
using shader_source_id_t = uint32_t;
using shader_source_id_t = uint32_t;
enum class ShaderSourceType {
    Vertex,
    Fragment,
    Geometry,
    Compute,
};

enum class TextureTarget {
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D_ARRAY,
    TEXTURE_RECTANGLE,
    TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY,
    TEXTURE_BUFFER,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_MULTISAMPLE_ARRAY
};

enum class TextureFormat {
    RED,
    RG,
    RGB,
    BGR,
    RGBA,
    BGRA,
    DEPTH_COMPONENT,
    DEPTH_STENCIL
};

enum class TextureWrap {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
};

enum class TextureFilter {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};
enum class ClearBufferMask {
    COLOR_BUFFER_BIT = 1 << 0,
    DEPTH_BUFFER_BIT = 1 << 1,
    STENCIL_BUFFER_BIT = 1 << 2,
};
template <class E>
class Flag {
    using underlying_type = std::underlying_type_t<E>;
   public:
    Flag() = default;
    Flag(E value) : value_(value) {}
    operator E() const { return value_; }
    bool test(E mask) const {
        return (static_cast<underlying_type>(value_) & static_cast<underlying_type>(mask)) != 0;
    }

   private:
    E value_;
};
using ClearBufferMaskFlag = Flag<ClearBufferMask>;
inline ClearBufferMaskFlag operator|(ClearBufferMask a, ClearBufferMask b) {
    return static_cast<ClearBufferMask>(static_cast<int>(a) |
                                        static_cast<int>(b));
}

}  // namespace xc::opengl