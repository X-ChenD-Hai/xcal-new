#include "render.hpp"

#include <cstdint>
#include <memory>
#include <print>

#include "ecs/event_bus.hpp"
#include "ecs/world.hpp"
#include "opengl_support.hpp"
#include "opengl_wrapper/buffer.hpp"
#include "opengl_wrapper/draw.hpp"
#include "opengl_wrapper/shader_program.hpp"
#include "opengl_wrapper/vertex_array.hpp"
#include "xcal2/camera/camera.hpp"
#include "xcal2/camera/fps_camera_controler.hpp"
#include "xcal2/camera/ui_controler/fps_ui_controler.hpp"
#include "xcal2/events/events.hpp"
#include "xcal2/transform/transform.hpp"

namespace opengl = opengl_support;
namespace app {

namespace shader_source_string {
const char* kVertexWithPosColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
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

static const char* kVertexWhitPosUniformColorShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;
uniform vec3 aColor;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * transform * vec4(aPos, 1.0);
    ourColor = aColor;
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
}  // namespace shader_source_string
namespace static_buffer_data {

static constexpr float kCubeVerticesPosition[]{
    // positions
    0.5f,  0.5f,  0.5f,   // top right front
    -0.5f, 0.5f,  0.5f,   // top left front
    0.5f,  -0.5f, 0.5f,   // bottom right front
    -0.5f, -0.5f, 0.5f,   // bottom left front
    0.5f,  0.5f,  -0.5f,  // top right back
    -0.5f, 0.5f,  -0.5f,  // top left back
    0.5f,  -0.5f, -0.5f,  // bottom right back
    -0.5f, -0.5f, -0.5f,  // bottom left back
};

static constexpr float kCubeVerticesColor[]{
    // colors
    1.0f, 0.0f, 0.0f,  // top right front
    0.0f, 1.0f, 0.0f,  // top left front
    0.0f, 0.0f, 1.0f,  // bottom right front
    1.0f, 1.0f, 0.0f,  // bottom left front
    1.0f, 0.0f, 1.0f,  // top right back
    0.0f, 1.0f, 1.0f,  // top left back
    1.0f, 1.0f, 1.0f,  // bottom right back
    0.0f, 0.0f, 0.0f,  // bottom left back
};

static constexpr uint32_t kCubeIndices[]{
    0, 1, 2,  // first triangle
    1, 3, 2,  // second triangle
    2, 3, 6,  // third triangle
    3, 7, 6,  // fourth triangle
    6, 7, 4,  // fifth triangle
    7, 5, 4,  // sixth triangle
    4, 5, 0,  // seventh triangle
    5, 1, 0,  // eighth triangle
    0, 3, 1,  // ninth triangle
    3, 2, 1,  // tenth triangle
    4, 6, 5,  // eleventh triangle
    6, 7, 5,  // twelfth triangle
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
using xcal::camera::ui_controler::FPSUIControler;
struct RenderHandle {
    std::unique_ptr<xc::opengl::ShaderProgram> program;
    std::unique_ptr<Cube> cube;
    std::unique_ptr<Trangle> trangle;
    std::unique_ptr<xcal::camera::FpsCameraControler> camera_controler;
    xcal::transform::TransformComponent transform{};
    RenderHandle(ecs::World& world) {
        using namespace xc::opengl;
        auto& view = world.resource<xcal::camera::ViewConfig>();
        camera_controler = std::make_unique<xcal::camera::FpsCameraControler>(
            &world.resource<ecs::EventBus>(),
            &world.resource<xcal::camera::ViewConfig>(),
            &world.resource<xcal::camera::ProjectionConfig>());
        world.add_resource(camera_controler.get());
        std::println("create shader program");
        try {
            program = std::unique_ptr<ShaderProgram>(new ShaderProgram{{
                {.source =
                     shader_source_string::kVertexWithPosColorShaderSource,
                 .type = ShaderSourceType::Vertex},
                {.source = shader_source_string::kFragmentShaderSource,
                 .type = ShaderSourceType::Fragment},
            }});

            std::println("create shader program {}", program->id());
        } catch (const std::exception& e) {
            std::println("create shader program failed: {}", e.what());
        }
        cube = std::make_unique<Cube>();
        trangle = std::make_unique<Trangle>();
        transform.position = {0.0f, 0.0f, -1.0f};
    }
    void draw(ecs::EventBus& event_bus, ecs::World& world) {
        program->use();
        update_camera(event_bus, world);
        // cube->draw();
        // trangle->draw();
        cube->draw();
    }

    void update_camera(ecs::EventBus& event_bus, ecs::World& world) {
        if (event_bus.exist<xcal::events::CameraProjectionChanged>()) {
            std::println("CameraProjectionChanged");
            program->uniform_mat4(
                "projection", world.resource<xcal::camera::ProjectionConfig>()
                                  .as_mat4()
                                  .T()
                                  .value_ptr());
        }
        if (event_bus.exist<xcal::events::CameraViewChanged>()) {
            std::println("CameraViewChanged\np:{}\nu:{}\nd:{}",
                         world.resource<xcal::camera::ViewConfig>().position,
                         world.resource<xcal::camera::ViewConfig>().up,
                         world.resource<xcal::camera::ViewConfig>().direction);
            program->uniform_mat4("view",
                                  world.resource<xcal::camera::ViewConfig>()
                                      .as_mat4()
                                      .T()
                                      .value_ptr());
        }
        program->uniform_mat4("transform", transform.to_mat().T().value_ptr());
    }
};
}  // namespace app

void app::Renderer::init(ecs::World& world) {
    world.use_plugin<xcal::camera::CameroPlugin>()
        .use_plugin<xcal::transform::TransformPlugin>()
        .use_plugin<FPSUIControler>();

    world.add_resource<Renderer>(std::make_unique<RenderHandle>(world));
    world.resource<FPSUIControler>()
        .set_dtranslation(0.05, 0.05, 0.05)
        .set_drotation(0.1, 0.1)
        .set_dzoom(0.01);
}
void app::Renderer::run(ecs::World& world, ecs::EventBus& event_bus) {
    using namespace xc::opengl;
    clear(ClearBufferMask::COLOR_BUFFER_BIT |
          ClearBufferMask::DEPTH_BUFFER_BIT);
    world.run_plugin<FPSUIControler>();
    handle_->draw(event_bus, world);

    event_bus.each([&](opengl::FrameResizeEvent& e) {
        std::println("FrameResizeEvent {} {}", e.width, e.height);
        viewport(0, 0, e.width, e.height);
    });
}
app::Renderer::Renderer(std::unique_ptr<RenderHandle> render_handle)
    : handle_(std::move(render_handle)) {}
