#include "render.hpp"

#include <memory>
#include <print>

#include "ecs/world.hpp"
#include "opengl_support.hpp"
#include "opengl_wrapper/buffer.hpp"
#include "opengl_wrapper/draw.hpp"
#include "opengl_wrapper/shader_program.hpp"
#include "opengl_wrapper/vertex_array.hpp"
#include "xcal2/camera/camera.hpp"
#include "xcal2/camera/fps_camera_controler.hpp"
#include "xcal2/events/events.hpp"
namespace opengl = opengl_support;
namespace app {
struct RenderHandle {
    std::unique_ptr<xc::opengl::ShaderProgram> program;
    xc::opengl::VertexArray vao;
    xc::opengl::Buffer vbo;
    std::unique_ptr<xcal::camera::FpsCameraControler> camera_controler;
    RenderHandle(ecs::World& world) {}

    void update_camera(ecs::EventBus& event_bus, ecs::World& world) {
        if (event_bus.exist<xcal::events::CameraProjectionChanged>()) {
            program->uniform_mat4(
                "projection",
                &world.resource<xcal::camera::ProjectionConfig>().as_mat4()[0]);
        }
        if (event_bus.exist<xcal::events::CameraViewChanged>()) {
            program->uniform_mat4(
                "view",
                &world.resource<xcal::camera::ViewConfig>().as_mat4()[0]);
        }
    }
};

// 顶点着色器源码
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 view;
uniform mat4 projection;
out vec3 ourColor;
void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    ourColor = aColor;
}
)glsl";

// 片段着色器源码
const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)glsl";
}  // namespace app
void app::Renderer::init(ecs::World& world) {
    world.use_plugin<xcal::camera::CameroPlugin>();

    std::unique_ptr<RenderHandle> handle =
        std::make_unique<RenderHandle>(world);
    using namespace xc::opengl;
    std::println("create shader program");
    try {
        handle->program = std::unique_ptr<ShaderProgram>(new ShaderProgram{{
            {.source = vertexShaderSource, .type = ShaderSourceType::Vertex},
            {.source = fragmentShaderSource,
             .type = ShaderSourceType::Fragment},
        }});

        std::println("create shader program {}", handle->program->id());
    } catch (const std::exception& e) {
        std::println("create shader program failed: {}", e.what());
    }

    // Draw a triangle
    float vertices[] = {
        // positions         // colors
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom left
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f   // top
    };
    // Create and bind a vertex array object (VAO)
    VertexArray& vao = handle->vao;
    vao.bind();
    std::println("create vertex array object {}", vao.id());
    Buffer& vbo = handle->vbo;
    vbo.bind(BufferTarget::ARRAY);
    std::println("create vertex buffer object {}", vbo.id());
    vbo.buffer_data(vertices, sizeof(vertices), BufferTarget::ARRAY,
                    BufferUsage::STATIC_DRAW);
    // Set vertex attribute pointers
    vao.set_attribute(0, 3, DataType::FLOAT, false, 6 * sizeof(float), nullptr);
    vao.set_attribute(1, 3, DataType::FLOAT, false, 6 * sizeof(float),
                      (void*)(3 * sizeof(float)));
    // Enable vertex attributes
    vao.enable_attribute(0);
    vao.enable_attribute(1);
    world.add_resource<Renderer>(std::move(handle));
}
void app::Renderer::run(ecs::World& world, ecs::EventBus& event_bus) {
    using namespace xc::opengl;
    clear(ClearBufferMask::COLOR_BUFFER_BIT |
          ClearBufferMask::DEPTH_BUFFER_BIT);
    handle_->vao.bind();
    handle_->program->use();
    handle_->update_camera(event_bus, world);

    draw_arrays(DrawMode::TRIANGLES, 0, 3);
    event_bus.each([&](opengl::FrameResizeEvent& e) {
        std::println("FrameResizeEvent {} {}", e.width, e.height);
        viewport(0, 0, e.width, e.height);
    });
}
app::Renderer::Renderer(std::unique_ptr<RenderHandle> render_handle)
    : handle_(std::move(render_handle)) {}
