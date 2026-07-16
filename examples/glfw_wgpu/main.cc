#include <glfw3webgpu.h>
#include <cstddef>
#include <string_view>

#include "./glfw.hpp"
#include "webgpu.hpp"

using namespace xc;
static const char* get_wgsl_source(void) {
    return R"wgsl(
struct Uniforms {
    transform : mat3x3<f32>,    // 旋转矩阵，对齐到 16 字节/列
};
@group(0) @binding(0) var<uniform> ubo : Uniforms;

struct VertexInput {
    @location(0) position : vec2<f32>,   // 顶点位置
    @location(1) color    : vec3<f32>,   // 顶点颜色
};

struct VertexOutput {
    @builtin(position) pos   : vec4<f32>,
    @location(0)       color : vec3<f32>,
};

@vertex
fn vs_main(in : VertexInput) -> VertexOutput {
    let transformed = ubo.transform * vec3(in.position, 1.0);
    var out : VertexOutput;
    out.pos   = vec4(transformed.xy, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4<f32> {
    return vec4(in.color, 1.0);         // 输出顶点颜色
}

@fragment
fn fs_outline() -> @location(0) vec4<f32> {
    return vec4(1.0, 1.0, 1.0, 1.0);   // 输出白色（描边用）
}
)wgsl";
}

#define exit_if_null(var, msg)  \
    do                          \
        if (!var) {             \
            XC_LOG(ERROR, msg); \
            return -1;          \
        }                       \
    while (0)
int main() {
    using STR = wgpu::StringView;
    XC_INIT_LOG("main");
    XC_LOG(INFO, "init app");
    GlfwInstence glfw{};
    GlfwWindow window(800, 600, "xcal2");
    exit_if_null(window, "create window failed");

    auto idesc = wgpu::InstanceDescriptor{};
    auto instence = wgpu::createInstance(idesc);
    exit_if_null(instence, "create instance failed");
    auto surface = wgpu::Surface(glfwCreateWindowWGPUSurface(
        (WGPUInstance)instence, window.raw_window()));
    exit_if_null(surface, "create surface failed");

    auto adesc = wgpu::RequestAdapterOptions{};
    auto adapter = instence.requestAdapter(adesc);
    exit_if_null(adapter, "create adapter failed");

    auto ddesc = wgpu::DeviceDescriptor{};
    auto device = adapter.requestDevice(ddesc);
    exit_if_null(device, "create device failed");

    auto queue = device.getQueue();

    auto scfg = wgpu::SurfaceConfiguration{};
    scfg.device = device;
    scfg.format = wgpu::TextureFormat::RGBA8Unorm;
    scfg.width = window.width();
    scfg.height = window.height();
    scfg.presentMode = wgpu::PresentMode::Fifo;
    scfg.alphaMode = wgpu::CompositeAlphaMode::Auto;
    scfg.usage = wgpu::TextureUsage::RenderAttachment;
    surface.configure(scfg);

    const char* wgsl_src = get_wgsl_source();

    auto wgsl_desc = wgpu::ShaderSourceWGSL{};
    wgsl_desc.code = {.data = wgsl_src, .length = strlen(wgsl_src)};
    wgsl_desc.chain = {.next = nullptr, .sType = wgpu::SType::ShaderSourceWGSL};

    auto wgsl_mod_desc = wgpu::ShaderModuleDescriptor{};
    wgsl_mod_desc.nextInChain = &wgsl_desc.chain;
    wgsl_mod_desc.label = STR{"Combined Shader"};

    auto shader_module = device.createShaderModule(wgsl_mod_desc);
    exit_if_null(shader_module, "create shader module failed");
    typedef struct {
        float x, y;
        float r, g, b;
    } Vertex;
    const Vertex vertices[] = {
        {0.0f, 0.5f, 1.0f, 0.0f, 0.0f},    // 顶点0: 红色
        {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f},  // 顶点1: 绿色
        {0.5f, -0.5f, 0.0f, 0.0f, 1.0f},   // 顶点2: 蓝色
    };
    const uint16_t indices_fill[] = {0, 1, 2, 0};           // 三角形填充索引
    const uint16_t indices_outline[] = {0, 1, 1, 2, 2, 0};  // 三角形描边索引

    auto bdesc = wgpu::BufferDescriptor{};
    bdesc.label = STR{"Vertex Buffer"};
    bdesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    bdesc.size = sizeof(vertices);
    bdesc.mappedAtCreation = false;

    auto vb = device.createBuffer(bdesc);
    exit_if_null(vb, "create vertex buffer failed");
    queue.writeBuffer(vb, 0, vertices, sizeof(vertices));

    bdesc.label = STR{"Fill Index Buffer"};
    bdesc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
    bdesc.size = sizeof(indices_fill);

    auto ib_fill = device.createBuffer(bdesc);
    exit_if_null(ib_fill, "create Fill index buffer failed");
    queue.writeBuffer(ib_fill, 0, indices_fill, sizeof(indices_fill));

    bdesc.label = STR{"Outline Index Buffer"};
    bdesc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
    bdesc.size = sizeof(indices_outline);

    auto ib_outline = device.createBuffer(bdesc);
    exit_if_null(ib_outline, "create Outline index buffer failed");
    queue.writeBuffer(ib_outline, 0, indices_outline, sizeof(indices_outline));

    float identity[12] = {0};
    identity[0] = 1.0f;
    identity[4] = 1.0f;
    identity[8] = 1.0f;  // 单位阵

    bdesc.label = STR{"Uniform Buffer"};
    bdesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    bdesc.size = sizeof(identity);

    auto ub = device.createBuffer(bdesc);
    exit_if_null(ub, "create uniform buffer failed");
    queue.writeBuffer(ub, 0, identity, sizeof(identity));

    auto bgl_entry = wgpu::BindGroupLayoutEntry{};
    bgl_entry.binding = 0;
    bgl_entry.visibility = wgpu::ShaderStage::Vertex;
    bgl_entry.buffer = {
        .nextInChain = nullptr,
        .type = wgpu::BufferBindingType::Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(identity),
    };

    auto bgl_desc = wgpu::BindGroupLayoutDescriptor{};
    bgl_desc.entries = &bgl_entry;
    bgl_desc.label = STR{"Uniform BGL"};
    bgl_desc.entryCount = 1;
    auto bgl = device.createBindGroupLayout(bgl_desc);
    exit_if_null(bgl, "create bind group layout failed");

    auto bg_entry = wgpu::BindGroupEntry{};
    bg_entry.binding = 0;
    bg_entry.buffer = ub;
    bg_entry.offset = 0;
    bg_entry.size = sizeof(identity);

    auto bg_desc = wgpu::BindGroupDescriptor{};
    bg_desc.label = STR{"Uniform BG"};
    bg_desc.layout = bgl;
    bg_desc.entryCount = 1;
    bg_desc.entries = &bg_entry;

    auto bg = device.createBindGroup(bg_desc);
    exit_if_null(bg, "create bind group failed");

    auto layout_desc = wgpu::PipelineLayoutDescriptor{};

    layout_desc.label = STR{"Shared Pipeline Layout"};
    layout_desc.bindGroupLayoutCount = 1;
    layout_desc.bindGroupLayouts = &(WGPUBindGroupLayout&)bgl;

    auto layout = device.createPipelineLayout(layout_desc);
    exit_if_null(layout, "create pipeline layout failed");

    auto attr1 = wgpu::VertexAttribute{};
    attr1.format = wgpu::VertexFormat::Float32x2;
    attr1.offset = 0;
    attr1.shaderLocation = 0;

    auto attr2 = wgpu::VertexAttribute{};
    attr2.format = wgpu::VertexFormat::Float32x3;
    attr2.offset = 2 * sizeof(float);
    attr2.shaderLocation = 1;
    wgpu::VertexAttribute attr[] = {attr1, attr2};

    auto vb_layout = wgpu::VertexBufferLayout{};
    vb_layout.arrayStride = sizeof(Vertex);
    vb_layout.stepMode = wgpu::VertexStepMode::Vertex;
    vb_layout.attributes = attr;
    vb_layout.attributeCount = 2;

    auto fill_pipline_desc = wgpu::RenderPipelineDescriptor{};
    fill_pipline_desc.label = STR{"Fill Pipeline"};
    fill_pipline_desc.layout = layout;
    fill_pipline_desc.vertex = {
        .nextInChain = nullptr,
        .module = shader_module,
        .entryPoint = STR{"vs_main"},
        .constantCount = 0,
        .constants = nullptr,
        .bufferCount = 1,
        .buffers = &vb_layout,
    };
    fill_pipline_desc.primitive = {
        .nextInChain = nullptr,
        .topology = wgpu::PrimitiveTopology::TriangleList,
        .stripIndexFormat = wgpu::IndexFormat::Undefined,
        .frontFace = wgpu::FrontFace::CCW,
        .cullMode = wgpu::CullMode::None};
    fill_pipline_desc.depthStencil = nullptr;
    fill_pipline_desc.multisample = {
        .nextInChain = nullptr,
        .count = 1,
        .mask = ~0u,
        .alphaToCoverageEnabled = false,
    };
    auto color_state = wgpu::ColorTargetState{};
    color_state.blend = nullptr,
    color_state.format = wgpu::TextureFormat::RGBA8Unorm;
    color_state.writeMask = wgpu::ColorWriteMask::All;
    auto fragment_state = wgpu::FragmentState{};
    fragment_state.module = shader_module;
    fragment_state.entryPoint = STR{"fs_main"};
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_state;
    fill_pipline_desc.fragment = &fragment_state;
    auto fill_pipline = device.createRenderPipeline(fill_pipline_desc);
    exit_if_null(fill_pipline, "create Fill pipeline failed");

    auto outline_pipline_desc = wgpu::RenderPipelineDescriptor{};
    outline_pipline_desc.label = STR{"Outline Pipeline"};
    outline_pipline_desc.layout = layout;
    outline_pipline_desc.vertex = {
        .nextInChain = nullptr,
        .module = shader_module,
        .entryPoint = STR{"vs_main"},
        .constantCount = 0,
        .constants = nullptr,
        .bufferCount = 1,
        .buffers = &vb_layout,
    };
    outline_pipline_desc.primitive = {
        .nextInChain = nullptr,
        .topology = wgpu::PrimitiveTopology::LineList,
        .stripIndexFormat = wgpu::IndexFormat::Undefined,
        .frontFace = wgpu::FrontFace::CCW,
        .cullMode = wgpu::CullMode::None};
    outline_pipline_desc.depthStencil = nullptr;
    outline_pipline_desc.multisample = {
        .nextInChain = nullptr,
        .count = 1,
        .mask = ~0u,
        .alphaToCoverageEnabled = false,
    };
    fragment_state.entryPoint = STR{"fs_outline"};
    outline_pipline_desc.fragment = &fragment_state;
    auto outline_pipline = device.createRenderPipeline(outline_pipline_desc);
    exit_if_null(outline_pipline, "create Fill pipeline failed");

    XC_LOG(INFO, "wgpu init ok");

    double start_time = glfwGetTime();

    auto main_enc_desc = wgpu::CommandEncoderDescriptor{};
    main_enc_desc.label = STR{"Main Encoder"};
    while (!window.should_close()) {
        glfwPollEvents();

        double elapsed = glfwGetTime() - start_time;
        float angle = (float)elapsed;
        float c = cosf(angle), s = sinf(angle);
        float M[12] = {
            c,  s, 0, 0,  //
            -s, c, 0, 0,  //
            0,  0, 1, 0,  //
        };
        queue.writeBuffer(ub, 0, M, sizeof(M));

        wgpu::SurfaceTexture st;
        surface.getCurrentTexture(&st);
        if (st.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal) {
            XC_LOG(WARN, "surface get current texture failed, status: %d",
                   (size_t)st.status);
            continue;
        }
        auto texture = wgpu::Texture{st.texture};
        auto view = texture.createView();
        exit_if_null(view, "create texture view failed");

        auto encoder = device.createCommandEncoder(main_enc_desc);
        exit_if_null(encoder, "create command encoder failed");

        auto color_att = wgpu::RenderPassColorAttachment{};
        color_att.view = view;
        color_att.resolveTarget = nullptr;
        color_att.loadOp = wgpu::LoadOp::Clear;
        color_att.storeOp = wgpu::StoreOp::Store;
        color_att.clearValue = {.r = 0.05f, .g = 0.05f, .b = 0.1f, .a = 1.0f};
        color_att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        auto pass_desc = wgpu::RenderPassDescriptor{};
        pass_desc.colorAttachmentCount = 1;
        pass_desc.colorAttachments = &color_att;

        auto pass_fill = encoder.beginRenderPass(pass_desc);
        exit_if_null(pass_fill, "begin render pass failed");

        pass_fill.setPipeline(fill_pipline);
        pass_fill.setBindGroup(0, bg, 0, 0);
        pass_fill.setVertexBuffer(0, vb, 0, sizeof(vertices));
        pass_fill.setIndexBuffer(ib_fill, wgpu::IndexFormat::Uint16, 0,
                                 sizeof(indices_fill));
        pass_fill.drawIndexed(3, 1, 0, 0, 0);
        pass_fill.end();

        color_att.loadOp = wgpu::LoadOp::Load;

        pass_desc.label = STR{"RenderPass Outline"};
        auto pass_outline = encoder.beginRenderPass(pass_desc);

        pass_outline.setPipeline(outline_pipline);
        pass_outline.setBindGroup(0, bg, 0, 0);
        pass_outline.setVertexBuffer(0, vb, 0, sizeof(vertices));
        pass_outline.setIndexBuffer(ib_outline, wgpu::IndexFormat::Uint16, 0,
                                    sizeof(indices_outline));
        pass_outline.drawIndexed(6, 1, 0, 0, 0);
        pass_outline.end();

        auto cmd_buf_desc = wgpu::CommandBufferDescriptor{};
        cmd_buf_desc.label = STR{"Main Command Buffer"};
        auto cmd_buf = encoder.finish(cmd_buf_desc);
        exit_if_null(cmd_buf, "finish command encoder failed");

        queue.submit(1, &cmd_buf);
        surface.present();

        window.swap_buffer();
    }

    XC_LOG(INFO, "app exit");
    return 0;
}