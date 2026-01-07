#include <cstdint>
namespace xcal_opengl_render {

namespace static_buffer_data {

constexpr float kCubeVerticesWithNormal[] = {
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,  //
    0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,  //
    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,  //
    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,  //
    -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,  //
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,  //

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  //
    0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  //
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  //
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  //
    -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  //
    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  //

    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  //
    -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,  //
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  //
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  //
    -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  //
    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  //

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  //
    0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,  //
    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  //
    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  //
    0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  //
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  //

    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  //
    0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  //
    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  //
    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  //
    -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  //
    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  //

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  //
    0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  //
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  //
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  //
    -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  //
    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f   //
};
constexpr float kCubeVerticesPosition[]{
    // positions
    -0.5f, -0.5f, -0.5f,  // 0: left bottom back
    0.5f,  -0.5f, -0.5f,  // 1: right bottom back
    0.5f,  0.5f,  -0.5f,  // 2: right top back
    -0.5f, 0.5f,  -0.5f,  // 3: left top back
    -0.5f, -0.5f, 0.5f,   // 4: left bottom front
    0.5f,  -0.5f, 0.5f,   // 5: right bottom front
    0.5f,  0.5f,  0.5f,   // 6: right top front
    -0.5f, 0.5f,  0.5f,   // 7: left top front
};
constexpr float kCubeVerticesColor[]{
    // colors
    1.0f, 0.0f, 0.0f,  // 0: red
    0.0f, 1.0f, 0.0f,  // 1: green
    0.0f, 0.0f, 1.0f,  // 2: blue
    1.0f, 1.0f, 0.0f,  // 3: yellow
    1.0f, 0.0f, 1.0f,  // 4: magenta
    0.0f, 1.0f, 1.0f,  // 5: cyan
    1.0f, 1.0f, 1.0f,  // 6: white
    0.5f, 0.5f, 0.5f,  // 7: gray
};
constexpr uint32_t kCubeIndices[]{
    0, 1, 2, 2, 3, 0,  // 背面
    4, 5, 6, 6, 7, 4,  // 前面
    3, 0, 4, 4, 7, 3,  // 左面
    1, 5, 6, 6, 2, 1,  // 右面
    0, 1, 5, 5, 4, 0,  // 底面
    3, 2, 6, 6, 7, 3   // 顶面
};
constexpr float kTrangleVerticesPosition[]{
    // positions
    0.0f,  0.5f,  0.0f,  // top
    -0.5f, -0.5f, 0.0f,  // bottom left
    0.5f,  -0.5f, 0.0f,  // bottom right
};
constexpr float kTrangleVerticesColor[]{
    // colors
    1.0f, 0.0f, 0.0f,  // top
    0.0f, 1.0f, 0.0f,  // bottom left
    0.0f, 0.0f, 1.0f,  // bottom right
};
}  // namespace static_buffer_data
}  // namespace xcal_opengl_render