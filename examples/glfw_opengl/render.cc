#include "render.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>
#include <utility>
#include <vector>

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
#include "xcal_opengl_render/render.hpp"
#include "xcal_opengl_render/shader.hpp"
#include "xcal_opengl_render/static_mesh_data.hpp"
#include "xcmath/mobject/declaration.hpp"
#include "xcmath/mobject/function.hpp"

namespace opengl = opengl_support;
namespace app {
using namespace xcal::object;
using ecs::core::Clock;
using GLRednder = xcal_opengl_render::Render;
namespace static_buffer_data = xcal_opengl_render::static_buffer_data;
namespace glrednder = xcal_opengl_render;
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
    bool closed{false};
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
        if (closed) {
            draw_arrays(DrawMode::LINE_LOOP, 0, count);
        } else {
            draw_arrays(DrawMode::LINE_STRIP, 0, count);
        }
    }
};

struct MeshSurface {
    xc::opengl::VertexArray vao{};
    xc::opengl::Buffer pos_vbo{};
    xc::opengl::Buffer ebo{};  // Index buffer
    std::vector<std::pair<uint32_t, uint32_t>> mesh_draw_range;
    std::vector<std::pair<uint32_t, uint32_t>> suface_draw_range;
    MeshSurface() {
        using namespace xc::opengl;
        vao.bind();
        std::println("create vertex array object {}", vao.id());
        // Bind position VBO
        pos_vbo.bind(BufferTarget::ARRAY);
        std::println("create vertex buffer object {}", pos_vbo.id());
        // Set vertex attribute pointers for position
        vao.set_attribute(0, 3, DataType::FLOAT, false, 3 * sizeof(float),
                          nullptr);
        vao.enable_attribute(0);
    }
    void draw_mesh() {
        using namespace xc::opengl;
        vao.bind();
        for (const auto& [offset, count] : mesh_draw_range) {
            draw_elements(DrawMode::LINE_STRIP, count,
                          (void*)(offset * sizeof(uint32_t)));
        }
    }
    void draw_surface() {
        using namespace xc::opengl;
        vao.bind();
        for (const auto& [offset, count] : suface_draw_range) {
            draw_elements(DrawMode::TRIANGLES, count,
                          (void*)(offset * sizeof(uint32_t)));
        }
    }
};

void dump(const QuadraticBezierCurve2d& bezier, Path& path, size_t count) {
    path.count = count;
    std::vector<xcmath::vec3f> vertices;
    vertices.reserve(path.count * 3);
    for (uint32_t i = 0; i < path.count; ++i) {
        float t = xcmath::lerp(0.0f, 1.0f, i / (path.count - 1.0f));
        vertices.push_back(lerp(lerp(bezier.p0, bezier.p1, t),
                                lerp(bezier.p1, bezier.p2, t), t));
    }
    path.vbo.bind(xc::opengl::BufferTarget::ARRAY);
    path.vbo.buffer_data(xc::opengl::BufferTarget::ARRAY,
                         xc::opengl::BufferUsage::STATIC_DRAW, vertices);
}
void dump(const FunctionCurve2d& obj, Path& path) {
    path.count = obj.num_samples;
    std::vector<float> vertices;
    vertices.reserve(obj.num_samples * 3);
    for (uint32_t i = 0; i < obj.num_samples; ++i) {
        double x = xcmath::lerp(obj.min_x, obj.max_x,
                                ((float)i) / (obj.num_samples - 1));
        double y = obj.function(x);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }
    path.vbo.bind(xc::opengl::BufferTarget::ARRAY);
    path.vbo.buffer_data(xc::opengl::BufferTarget::ARRAY,
                         xc::opengl::BufferUsage::STATIC_DRAW, vertices);
}
void dump(const ParametricCurve& obj, Path& path) {
    path.count = obj.num_samples;
    std::vector<xcmath::vec3f> vertices;
    vertices.reserve(obj.num_samples * 3);
    for (uint32_t i = 0; i < obj.num_samples; ++i) {
        float t = xcmath::lerp(obj.min_t, obj.max_t, i / (obj.num_samples - 1));
        vertices.push_back(obj.function(t));
    }
    path.vbo.bind(xc::opengl::BufferTarget::ARRAY);
    path.vbo.buffer_data(xc::opengl::BufferTarget::ARRAY,
                         xc::opengl::BufferUsage::STATIC_DRAW, vertices);
}
void dump(const ParametricSurface& obj, MeshSurface& surface) {
    std::vector<xcmath::vec3f> vertices;
    std::vector<uint32_t> indices;
    auto dot_count = obj.num_samples_u * obj.num_samples_v;
    auto tcount = (obj.num_samples_u - 1) * (obj.num_samples_v - 1) * 2;
    indices.reserve(dot_count * 2 + tcount * 3);
    vertices.reserve(obj.num_samples_u * obj.num_samples_v);
    for (uint32_t i = 0; i < obj.num_samples_u; ++i) {
        for (uint32_t j = 0; j < obj.num_samples_v; ++j) {
            float u = xcmath::lerp(obj.min_u, obj.max_u,
                                   (float)i / (obj.num_samples_u - 1));
            float v = xcmath::lerp(obj.min_v, obj.max_v,
                                   (float)j / (obj.num_samples_v - 1));
            vertices.push_back(obj.function(u, v));
            indices.push_back(i * obj.num_samples_v + j);
        }
        surface.mesh_draw_range.emplace_back(i * obj.num_samples_v,
                                             obj.num_samples_v);
    }

    for (uint32_t i = 0; i < obj.num_samples_v; ++i) {
        for (uint32_t j = 0; j < obj.num_samples_u; ++j) {
            indices.push_back(j * obj.num_samples_v + i);
        }
        surface.mesh_draw_range.emplace_back(dot_count + i * obj.num_samples_u,
                                             obj.num_samples_u);
    }
    surface.suface_draw_range.emplace_back(2 * dot_count, tcount * 3);
    for (uint32_t i = 0; i < obj.num_samples_u - 1; ++i) {
        for (uint32_t j = 0; j < obj.num_samples_v - 1; ++j) {
            // 计算四个顶点的索引
            uint32_t idx00 = i * obj.num_samples_v + j;        // 当前网格左下
            uint32_t idx10 = (i + 1) * obj.num_samples_v + j;  // 当前网格右下
            uint32_t idx01 = i * obj.num_samples_v + (j + 1);  // 当前网格左上
            uint32_t idx11 =
                (i + 1) * obj.num_samples_v + (j + 1);  // 当前网格右上

            // 第一个三角形（左下-右下-右上），顺时针或逆时针取决于渲染设置
            indices.push_back(idx00);
            indices.push_back(idx10);
            indices.push_back(idx11);

            // 第二个三角形（左下-右上-左上）
            indices.push_back(idx00);
            indices.push_back(idx11);
            indices.push_back(idx01);
        }
    }

    surface.pos_vbo.bind(xc::opengl::BufferTarget::ARRAY);
    surface.pos_vbo.buffer_data(xc::opengl::BufferTarget::ARRAY,
                                xc::opengl::BufferUsage::STATIC_DRAW, vertices);
    surface.ebo.bind(xc::opengl::BufferTarget::ELEMENT_ARRAY);
    surface.ebo.buffer_data(xc::opengl::BufferTarget::ELEMENT_ARRAY,
                            xc::opengl::BufferUsage::STATIC_DRAW, indices);
}

template <typename A, typename B>
inline void dump(const A& a, const std::unique_ptr<B>& b) {
    dump(a, *b.get());
}
template <typename A, typename B>
inline void dump(const A& a, const B* b) {
    dump(a, *b);
}

using xcal::camera::ui_controler::FPSUIControler;
struct RenderHandle {
    std::unique_ptr<Cube> cube;
    std::unique_ptr<NormelCube> normel_cube;
    std::unique_ptr<Trangle> trangle;
    std::unique_ptr<xcal::camera::FpsCameraControler> camera_controler;
    xcal::transform::TransformComponent transform_cube{};
    xcal::transform::TransformComponent transform_light{};
    std::unique_ptr<Path> path;
    std::unique_ptr<Path> diff_path;
    std::unique_ptr<Path> bezier_path;
    std::unique_ptr<MeshSurface> mesh_surface;
    xcmath::vec3f model_color{.0f, 0.9f, 0.9f};
    xcmath::vec3f mesh_color{.0f, 0.6f, 0.6f};
    xcmath::vec3f suface_color{.0f, 0.3f, 0.3f};
    xcmath::vec3f loght_color{1.0f, 1.0f, 1.0f};
    double offset;
    FunctionCurve2d curve;
    RenderHandle(ecs::World& world)
        : curve{[this](double x) { return std::sin((x) * 2 * 3.1415); }} {
        offset = 0;
        mesh_surface = std::make_unique<MeshSurface>();

        dump(ParametricSurface{[](float theta, float gama) {
                                   //    return xcmath::vec3f{theta, gama,
                                   //    0.0f};
                                   float r = 1;
                                   return r *
                                          xcmath::vec3f(
                                              std::sin(theta) * std::cos(gama),
                                              std::sin(theta) * std::sin(gama),
                                              std::cos(theta));
                               },
                               .0f, 3.14f, 50, .0f, 2. * 3.14, 50},
             *mesh_surface.get());

        using namespace xc::opengl;
        auto& view = world.resource<xcal::camera::ViewConfig>();
        camera_controler = std::make_unique<xcal::camera::FpsCameraControler>(
            &world.resource<ecs::EventBus>(),
            &world.resource<xcal::camera::ViewConfig>(),
            &world.resource<xcal::camera::ProjectionConfig>());
        world.add_resource(camera_controler.get());
        cube = std::make_unique<Cube>();
        normel_cube = std::make_unique<NormelCube>();
        trangle = std::make_unique<Trangle>();
        transform_cube.position = {0.0f, 0.0f, -1.0f};
        transform_light = transform_cube;
        transform_light.position = {1.f, 1.f, -1.0f};
        transform_light.scale = {0.3, 0.3, 0.3};
        path = std::make_unique<Path>();
        dump(ParametricCurve{[](float t) { return xcmath::vec3f(t, t, 2 * t); },
                             0.0f, 1.0f},
             path);
        diff_path = std::make_unique<Path>();
        dump(curve.derivative(), diff_path);
        bezier_path = std::make_unique<Path>();
        dump(
            QuadraticBezierCurve2d{
                {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {2.0f, 0.0f, 0.0f}},
            *bezier_path, 100);
        std::println("count {}", bezier_path->count);
    }
    void draw_light(ecs::EventBus& event_bus, ecs::World& world,
                    glrednder::ShaderTable& shader_table) {
        use_shader(event_bus, world,
                   shader_table.pos_then_uniform_color_shader.get());
        uniform_transform(shader_table.pos_then_uniform_color_shader.get(),
                          transform_light);
        shader_table.pos_then_uniform_color_shader->uniform_vec3(
            "color", loght_color.value_ptr());
        normel_cube->draw();
    }
    void draw(ecs::EventBus& event_bus, ecs::World& world) {
        draw_light(event_bus, world, world.resource<glrednder::ShaderTable>());
        use_shader(event_bus, world,
                   world.resource<glrednder::ShaderTable>()
                       .pos_then_uniform_color_shader.get());
        uniform_transform(world.resource<glrednder::ShaderTable>()
                              .pos_then_uniform_color_shader.get(),
                          transform_cube);
        world.resource<glrednder::ShaderTable>()
            .pos_then_uniform_color_shader->uniform_vec3(
                "color", mesh_color.value_ptr());
        mesh_surface->draw_mesh();
        world.resource<glrednder::ShaderTable>()
            .pos_then_uniform_color_shader->uniform_vec3(
                "color", suface_color.value_ptr());
        mesh_surface->draw_surface();
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
        .use_plugin<GLRednder>()
        .use_plugin<Clock>()
        .add_resource<Renderer>(std::make_unique<RenderHandle>(world));
    world.resource<FPSUIControler>()
        .set_dtranslation(0.05, 0.05, 0.05)
        .set_drotation(0.1, 0.1)
        .set_dzoom(0.01);
}
void app::Renderer::run(ecs::World& world, ecs::EventBus& event_bus) {
    using namespace xc::opengl;
    world.run_plugin<Clock>()
        .run_plugin<FPSUIControler>()
        .run_plugin<GLRednder>();
    clear(ClearBufferMask::COLOR_BUFFER_BIT |
          ClearBufferMask::DEPTH_BUFFER_BIT);
    handle_->transpose_event(event_bus, world);
    handle_->draw(event_bus, world);
}
app::Renderer::Renderer(std::unique_ptr<RenderHandle> render_handle)
    : handle_(std::move(render_handle)) {}

void app::RenderHandle::uniform_transform(
    xc::opengl::ShaderProgram* shader,
    xcal::transform::TransformComponent transform) {
    shader->uniform_mat4("transform", transform.to_mat().T().value_ptr());
}

void app::RenderHandle::use_shader(ecs::EventBus& event_bus, ecs::World& world,
                                   xc::opengl::ShaderProgram* shader) {
    shader->use();
}