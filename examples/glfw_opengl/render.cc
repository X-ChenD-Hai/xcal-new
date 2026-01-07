#include "render.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>

#include "ecs/event_bus.hpp"
#include "ecs/plugin/core/clock.hpp"
#include "ecs/world.hpp"
#include "opengl_support.hpp"
#include "opengl_wrapper/buffer.hpp"
#include "opengl_wrapper/draw.hpp"
#include "opengl_wrapper/shader_program.hpp"
#include "opengl_wrapper/types.hpp"
#include "opengl_wrapper/vertex_array.hpp"
#include "xcal2/camera/camera.hpp"
#include "xcal2/camera/fps_camera_controler.hpp"
#include "xcal2/camera/ui_controler/fps_ui_controler.hpp"
#include "xcal2/events/events.hpp"
#include "xcal2/object/types.hpp"
#include "xcal2/transform/transform.hpp"
#include "xcmath/mobject/declaration.hpp"

namespace opengl = opengl_support;
namespace app {
using namespace xcal::object;
using ecs::core::Clock;
namespace shader_source_string {
const char* kVertexWithPosUniformColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;
uniform vec3 color;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = color;
}
)glsl";
const char* kVertexWithPosColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 3) in vec3 aNormel;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = aColor;
}
)glsl";

static const char* kVertexWhitPosNormalLightShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aNormal;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;
uniform vec3 color;
out vec3 ourColor;
out vec3 normal;
out vec3 vpos;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = color;
    normal = mat3(transpose(inverse(transform))) * aNormal;;
    vpos = vec3(transform * vec4(aPos, 1.0));
}
)glsl";

// 片段着色器源码
const char* kFragmentShaderSource = R"glsl(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)glsl";
const char* kFragmentWithLightShaderSource = R"glsl(
#version 330 core
uniform vec3 light_color;
uniform vec3 light_pos;
uniform mat4 transform;
uniform float ambientStrength;
in vec3 ourColor;
in vec3 vpos;
in vec3 normal;
out vec4 FragColor;
void main()
{
    vec3 norm = normalize(normal);
    vec3 light_direction = normalize(light_pos-vpos);
    float diff = max(dot(norm, light_direction), 0.0);
    FragColor = vec4(ourColor*(ambientStrength+diff), 1.0);
}
)glsl";
}  // namespace shader_source_string
namespace static_buffer_data {

static constexpr float kCubeVerticesWithNormal[] = {
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
static constexpr float kCubeVerticesPosition[]{
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
static constexpr float kCubeVerticesColor[]{
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
static constexpr uint32_t kCubeIndices[]{
    0, 1, 2, 2, 3, 0,  // 背面
    4, 5, 6, 6, 7, 4,  // 前面
    3, 0, 4, 4, 7, 3,  // 左面
    1, 5, 6, 6, 2, 1,  // 右面
    0, 1, 5, 5, 4, 0,  // 底面
    3, 2, 6, 6, 7, 3   // 顶面
};
static constexpr float kTrangleVerticesPosition[]{
    // positions
    0.0f,  0.5f,  0.0f,  // top
    -0.5f, -0.5f, 0.0f,  // bottom left
    0.5f,  -0.5f, 0.0f,  // bottom right
};
static constexpr float kTrangleVerticesColor[]{
    // colors
    1.0f, 0.0f, 0.0f,  // top
    0.0f, 1.0f, 0.0f,  // bottom left
    0.0f, 0.0f, 1.0f,  // bottom right
};

}  // namespace static_buffer_data
struct Cube {
    xc::opengl::VertexArray vao{};
    xc::opengl::Buffer pos_vbo{};
    xc::opengl::Buffer color_vbo{};
    xc::opengl::Buffer ebo{};  // Index buffer
    Cube() {
        using namespace xc::opengl;

        vao.bind();
        std::println("create vertex array object {}", vao.id());

        // Bind position VBO
        pos_vbo.bind(BufferTarget::ARRAY);
        std::println("create vertex buffer object {}", pos_vbo.id());
        pos_vbo.buffer_data(BufferTarget::ARRAY, BufferUsage::STATIC_DRAW,
                            static_buffer_data::kCubeVerticesPosition);
        // Set vertex attribute pointers for position
        vao.set_attribute(0, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        vao.enable_attribute(0);

        // Bind color VBO
        color_vbo.bind(BufferTarget::ARRAY);
        std::println("create color buffer object {}", color_vbo.id());
        color_vbo.buffer_data(BufferTarget::ARRAY, BufferUsage::STATIC_DRAW,
                              static_buffer_data::kCubeVerticesColor);

        // Set vertex attribute pointers for color
        vao.set_attribute(1, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        // Enable vertex attributes
        vao.enable_attribute(1);

        // Create and bind index buffer
        ebo.bind(BufferTarget::ELEMENT_ARRAY);
        std::println("create element buffer object {}", ebo.id());
        ebo.buffer_data(BufferTarget::ELEMENT_ARRAY, BufferUsage::STATIC_DRAW,
                        static_buffer_data::kCubeIndices);
    }
    void draw() {
        using namespace xc::opengl;
        vao.bind();
        draw_elements(DrawMode::TRIANGLES, 36, nullptr);
    }
};

struct NormelCube {
    xc::opengl::VertexArray vao{};
    xc::opengl::Buffer vbo{};
    NormelCube() {
        using namespace xc::opengl;
        vao.bind();
        std::println("create vertex array object {}", vao.id());
        // Bind position VBO
        vbo.bind(BufferTarget::ARRAY);
        std::println("create vertex buffer object {}", vbo.id());
        vbo.buffer_data(BufferTarget::ARRAY, BufferUsage::STATIC_DRAW,
                        static_buffer_data::kCubeVerticesWithNormal);
        // Set vertex attribute pointers for position
        vao.set_attribute(0, 3, DataType::FLOAT, false, 6 * sizeof(float),
                          nullptr);
        vao.enable_attribute(0);
        // Set vertex attribute pointers for normal
        vao.set_attribute(3, 3, DataType::FLOAT, false, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
        vao.enable_attribute(3);
    }
    void draw() {
        using namespace xc::opengl;
        vao.bind();
        draw_arrays(DrawMode::TRIANGLES, 0, 36);
    }
};

struct Trangle {
    xc::opengl::VertexArray vao{};
    xc::opengl::Buffer pos_vbo{};
    xc::opengl::Buffer color_vbo{};
    Trangle() {
        using namespace xc::opengl;

        vao.bind();
        std::println("create vertex array object {}", vao.id());
        // Bind position VBO
        pos_vbo.bind(BufferTarget::ARRAY);
        std::println("create vertex buffer object {}", pos_vbo.id());
        pos_vbo.buffer_data(BufferTarget::ARRAY, BufferUsage::STATIC_DRAW,
                            static_buffer_data::kTrangleVerticesPosition);
        // Set vertex attribute pointers for position
        vao.set_attribute(0, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        vao.enable_attribute(0);
        // Bind color VBO
        color_vbo.bind(BufferTarget::ARRAY);
        std::println("create color buffer object {}", color_vbo.id());
        color_vbo.buffer_data(BufferTarget::ARRAY, BufferUsage::STATIC_DRAW,
                              static_buffer_data::kTrangleVerticesColor);
        // Set vertex attribute pointers for color
        vao.set_attribute(1, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        // Enable vertex attributes
        vao.enable_attribute(1);
    }
    void draw() {
        using namespace xc::opengl;
        vao.bind();
        draw_arrays(DrawMode::TRIANGLES, 0, 3);
    }
};

struct Path {
    xc::opengl::VertexArray vao{};
    xc::opengl::Buffer vbo{};
    size_t count{0};
    Path() {
        using namespace xc::opengl;
        vao.bind();
        std::println("create vertex array object {}", vao.id());
        // Bind position VBO
        vbo.bind(BufferTarget::ARRAY);
        std::println("create vertex buffer object {}", vbo.id());
        // Set vertex attribute pointers for position
        vao.set_attribute(0, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        vao.enable_attribute(0);
    }
    void draw() {
        using namespace xc::opengl;
        vao.bind();
        draw_arrays(DrawMode::LINE_STRIP, 0, count);
    }
};

void dump(const FunctionCurve2d& obj, Path& path) {
    path.count = obj.num_samples;
    std::vector<float> vertices;
    vertices.reserve(obj.num_samples * 3);
    for (uint32_t i = 0; i < obj.num_samples; ++i) {
        double x =
            obj.min_x + (obj.max_x - obj.min_x) * i / (obj.num_samples - 1);
        double y = obj.function(x);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }
    path.vbo.bind(xc::opengl::BufferTarget::ARRAY);
    path.vbo.buffer_data(xc::opengl::BufferTarget::ARRAY,
                         xc::opengl::BufferUsage::STATIC_DRAW, vertices);
}
using xcal::camera::ui_controler::FPSUIControler;
struct RenderHandle {
    std::unique_ptr<xc::opengl::ShaderProgram> vertex_color_shader;
    std::unique_ptr<xc::opengl::ShaderProgram> uniform_color_shader;
    std::unique_ptr<xc::opengl::ShaderProgram> uniform_color_and_light_shader;
    std::unique_ptr<Cube> cube;
    std::unique_ptr<NormelCube> normel_cube;
    std::unique_ptr<Trangle> trangle;
    std::unique_ptr<xcal::camera::FpsCameraControler> camera_controler;
    xcal::transform::TransformComponent transform_cube{};
    xcal::transform::TransformComponent transform_light{};
    std::unique_ptr<Path> path;
    xcmath::vec3f model_color{.0f, 1.0f, 1.0f};
    xcmath::vec3f loght_color{1.0f, 1.0f, 1.0f};
    RenderHandle(ecs::World& world) {
        using namespace xc::opengl;
        auto& view = world.resource<xcal::camera::ViewConfig>();
        camera_controler = std::make_unique<xcal::camera::FpsCameraControler>(
            &world.resource<ecs::EventBus>(),
            &world.resource<xcal::camera::ViewConfig>(),
            &world.resource<xcal::camera::ProjectionConfig>());
        world.add_resource(camera_controler.get());
        std::println("create shader programs");
        vertex_color_shader =
            make_shader(shader_source_string::kVertexWithPosColorShaderSource,
                        shader_source_string::kFragmentShaderSource);
        uniform_color_shader = make_shader(
            shader_source_string::kVertexWithPosUniformColorShaderSource,
            shader_source_string::kFragmentShaderSource);
        uniform_color_and_light_shader = make_shader(
            shader_source_string::kVertexWhitPosNormalLightShaderSource,
            shader_source_string::kFragmentWithLightShaderSource);
        cube = std::make_unique<Cube>();
        normel_cube = std::make_unique<NormelCube>();
        trangle = std::make_unique<Trangle>();
        transform_cube.position = {0.0f, 0.0f, -1.0f};
        transform_light = transform_cube;
        transform_light.position = {1.f, 1.f, -1.0f};
        transform_light.scale = {0.3, 0.3, 0.3};
        path = std::make_unique<Path>();
        FunctionCurve2d curve([](double x) { return std::sin(x * 10); });
        dump(curve, *path.get());
    }
    void draw_light(ecs::EventBus& event_bus, ecs::World& world) {
        use_shader(event_bus, world, uniform_color_shader.get());
        uniform_transform(uniform_color_shader.get(), transform_light);
        uniform_color_shader->uniform_vec3("color", loght_color.value_ptr());
        normel_cube->draw();
    }
    void draw(ecs::EventBus& event_bus, ecs::World& world) {
        draw_light(event_bus, world);
        // use_shader(event_bus, world, uniform_color_and_light_shader.get());
        // uniform_transform(uniform_color_and_light_shader.get(),
        // transform_cube);
        // uniform_color_and_light_shader->uniform_vec3("color",
        //                                              model_color.value_ptr());
        // uniform_color_and_light_shader->uniform("ambientStrength", 0.1f);
        // uniform_color_and_light_shader->uniform_vec3(
        //     "light_pos", transform_light.position.value_ptr());
        // uniform_color_and_light_shader->uniform_vec3("light_color",
        //                                              loght_color.value_ptr());
        // normel_cube->draw();
        use_shader(event_bus, world, uniform_color_shader.get());
        uniform_transform(uniform_color_shader.get(), transform_cube);
        uniform_color_shader->uniform_vec3("color", model_color.value_ptr());
        path->draw();
    }

    void transpose_event(ecs::EventBus& event_bus, ecs::World& world) {
        using namespace xc::opengl;
        event_bus.each([&](opengl::FrameResizeEvent& e) {
            std::println("FrameResizeEvent {} {}", e.width, e.height);
            viewport(0, 0, e.width, e.height);
            world.resource<xcal::camera::ProjectionConfig>().aspect =
                static_cast<float>(e.width) / static_cast<float>(e.height);
            event_bus.publish<xcal::events::CameraProjectionChanged>();
        });
    }
    static std::unique_ptr<xc::opengl::ShaderProgram> make_shader(
        const char* vertex_shader_source, const char* fragment_shader_source);
    static void uniform_transform(
        xc::opengl::ShaderProgram* shader,
        xcal::transform::TransformComponent transform);
    static void use_shader(ecs::EventBus& event_bus, ecs::World& world,
                           xc::opengl::ShaderProgram* shader);
};

}  // namespace app

void app::Renderer::init(ecs::World& world) {
    world.use_plugin<xcal::camera::Camero>()
        .use_plugin<xcal::transform::Transform>()
        .use_plugin<FPSUIControler>()
        .use_plugin<Clock>();
        
    world.add_resource<Renderer>(std::make_unique<RenderHandle>(world));
    world.resource<FPSUIControler>()
        .set_dtranslation(0.05, 0.05, 0.05)
        .set_drotation(0.1, 0.1)
        .set_dzoom(0.01);

    world.resource<Clock>().tick(0.5,[](){
        std::println("tick");
    } );
}
void app::Renderer::run(ecs::World& world, ecs::EventBus& event_bus) {
    using namespace xc::opengl;
    world.run_plugin<Clock>().run_plugin<FPSUIControler>();
    clear(ClearBufferMask::COLOR_BUFFER_BIT |
          ClearBufferMask::DEPTH_BUFFER_BIT);
    handle_->transpose_event(event_bus, world);
    handle_->draw(event_bus, world);
}
app::Renderer::Renderer(std::unique_ptr<RenderHandle> render_handle)
    : handle_(std::move(render_handle)) {}

std::unique_ptr<xc::opengl::ShaderProgram> app::RenderHandle::make_shader(
    const char* vertex_shader_source, const char* fragment_shader_source) {
    using namespace xc::opengl;
    try {
        auto shader = std::unique_ptr<ShaderProgram>(new ShaderProgram{{
            {.source = vertex_shader_source, .type = ShaderSourceType::Vertex},
            {.source = fragment_shader_source,
             .type = ShaderSourceType::Fragment},
        }});

        std::println("create shader program {}", shader->id());
        return shader;
    } catch (const std::exception& e) {
        std::println("create shader program failed: {}", e.what());
    }
    return nullptr;
}

void app::RenderHandle::uniform_transform(
    xc::opengl::ShaderProgram* shader,
    xcal::transform::TransformComponent transform) {
    shader->uniform_mat4("transform", transform.to_mat().T().value_ptr());
}

void app::RenderHandle::use_shader(ecs::EventBus& event_bus, ecs::World& world,
                                   xc::opengl::ShaderProgram* shader) {
    shader->use();
    if (event_bus.exist<xcal::events::CameraProjectionChanged>()) {
        std::println("CameraProjectionChanged");
        shader->uniform_mat4("projection",
                             world.resource<xcal::camera::ProjectionConfig>()
                                 .as_mat4()
                                 .T()
                                 .value_ptr());
    }
    if (event_bus.exist<xcal::events::CameraViewChanged>()) {
        shader->uniform_mat4("view", world.resource<xcal::camera::ViewConfig>()
                                         .as_mat4()
                                         .T()
                                         .value_ptr());
    }
}