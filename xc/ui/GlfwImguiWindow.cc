#include "./GlfwImguiWindow.hpp"
#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#define _gl gl::
#endif
#ifdef USE_GLAD
#include <glad/glad.h>
#define _gl
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
void Window::render_frame_() {
    loader_->make_current();
    _gl glClear(_gl GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Hello, world!");
    ImGui::SetWindowFontScale(2.f);
    ImGui::Text("This is some useful text.");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    loader_->swap_buffers();
}
bool Window::event(AbsEvent* event) {
    auto e = static_cast<Event*>(event);
    if (e->sender() == this) {
        if (e->type() == EventType::Update)
            return true;
        else if (e->type() == EventType::Render) {
            update_immediately_();
            return true;
        }
    } else if (e->sender() == frame_timer_.get()) {
        if (e->type() == EventType::TimeOut) {
            update();
            return true;
        }
    } else if (e->sender() == loader_.get()) {
        if (e->type() == EventType::WindowCloseRequested) {
            stop_flag_ = true;
            return true;
        } else if (e->type() == EventType::MouseMoved) {
            if (ImGui::GetIO().WantCaptureMouse) {
                update_immediately_();
                return true;
            }
            return mouse_move_event(static_cast<MouseMoveEvent*>(e));
        } else if (e->type() == EventType::MouseButtonPressed) {
            if (ImGui::GetIO().WantCaptureMouse) {
                update_immediately_();
                return true;
            }
            return mouse_button_event(static_cast<MouseButtonEvent*>(e));
        }
    }
    return false;
}
Window::Window() : loader_(std::make_unique<GlfwWindowLoader>(loop())) {
    loader_->make_current();
    glbinding::initialize(loader_->get_proc_address(), false);
    ImGui::CreateContext();
    auto io = &ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(loader_->glfw_window_raw_ptr(), true);
    ImGui_ImplOpenGL3_Init();
    _gl glClearColor(0.2f, 0.3f, 0.f, 1.0f);
}
Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
    loader_.reset();
}
