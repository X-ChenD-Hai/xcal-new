#include "./app_context.hpp"

#include <GLFW/glfw3.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

AppContext* AppContext::install(ecs::World& world) {
    if (!world.resource_manager().has<ecs::EventBus>())
        world.add_resource<ecs::EventBus>();
    auto p = new AppContext();
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
}

void AppContext::end_frame(ecs::EventBus& bus) const {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

    // Set window resize callback
    glfwSetWindowSizeCallback(window_,
                              [](GLFWwindow* window, int width, int height) {
                                  gl::glViewport(0, 0, width, height);
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

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

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
