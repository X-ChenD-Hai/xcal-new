#include "./app_context.hpp"

#include <glbinding/gl/gl.h>
#define __gl_h_
#include <GLFW/glfw3.h>
#include <glbinding/glbinding.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <xc/ecs/event_bus.hpp>

static inline AppContext* ctx_ptr(GLFWwindow* w) {
    return (AppContext*)glfwGetWindowUserPointer(w);
}

AppContext* AppContext::install(ecs::World& world) {
    if (!world.resource_manager().has<ecs::EventBus>())
        world.add_resource<ecs::EventBus>();
    auto p = new AppContext(world);
    p->initialize();
    return p;
}
void AppContext::uninstall(ecs::World& world, AppContext* p) { delete p; }
void AppContext::begin_frame() const {
    glfwMakeContextCurrent(window_);
    glfwPollEvents();

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ImGui::DockSpaceOverViewport();
}

void AppContext::end_frame(ecs::EventBus& bus) const {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call
    //  glfwMakeContextCurrent(window) directly)
    auto& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window_);
    if (glfwWindowShouldClose(window_)) {
        bus.publish<CloseRequestEvent>();
    }
}

bool AppContext::initialize() {
    if (!init_glfw()) {
        return false;
    }

    if (!init_glfw_window()) {
        return false;
    }
    load_opengl();
    if (!init_imgui()) {
        return false;
    }

    return true;
}

bool AppContext::init_glfw() {
    // Initialize GLFW
    if (!glfwInit()) {
        return false;
    }

    // Set GLFW version hint
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

bool AppContext::init_glfw_window() {
    // Create a GLFW window
    window_ = glfwCreateWindow(1280, 720, "GLFW ImGui Game", nullptr, nullptr);
    if (window_ == nullptr) {
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // Enable vsync

    glfwSetWindowUserPointer(window_, this);

    // Set window resize callback
    glfwSetWindowSizeCallback(
        window_, [](GLFWwindow* window, int width, int height) {
            ctx_ptr(window)
                ->world_.resource<ecs::EventBus>()
                .publish<AppContext::FrameResizeEvent>(width, height);
        });

    return true;
}

void AppContext::load_opengl() {
    glbinding::initialize(glfwGetProcAddress, false);
}

bool AppContext::init_imgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    // io.ConfigFlags |=
    // ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform
    // Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    // style.ScaleAllSizes(main_scale);        // Bake a fixed style scale.
    // (until we have a solution for dynamic style scaling, changing this
    // requires resetting Style + calling this again) style.FontScaleDpi =
    // main_scale;        // Set initial font scale. (using
    // io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here
    // for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
    // io.ConfigDpiScaleFonts =
    //     true;  // [Experimental] Automatically overwrite style.FontScaleDpi
    //     in
    //            // Begin() when Monitor DPI changes. This will scale fonts but
    //            // _NOT_ scale sizes/padding for now.
    // io.ConfigDpiScaleViewports =
    //     true;  // [Experimental] Scale Dear ImGui and Platform Windows when
    //            // Monitor DPI changes.
#endif
    io.FontGlobalScale = 1.8;
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void AppContext::cleanup() {
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    glfwDestroyWindow(window_);
    glfwTerminate();
}
