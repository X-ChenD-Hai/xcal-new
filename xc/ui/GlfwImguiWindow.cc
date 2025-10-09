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

static bool render_editor_field(EditorField& field) {
    return std::visit(
        [&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::string>) {
                static char buf[1024];
                fill(buf, buf + 1024, '\0');
                if (!ImGui::InputText(field.name.c_str(), buf, 1024))
                    return false;
                field.value = std::string(buf);
                return true;
            } else if constexpr (std::is_same_v<T, int>) {
                return ImGui::InputInt(field.name.c_str(), &val);
            } else if constexpr (std::is_same_v<T, float>) {
                return ImGui::InputFloat(field.name.c_str(), &val);
            } else if constexpr (std::is_same_v<T, bool>) {
                return ImGui::Checkbox(field.name.c_str(), &val);
            }
            return false;
        },
        field.value);
}

static void render_editor(std::vector<std::unique_ptr<Editor>>& editors) {
    for (int i; i < editors.size(); i++) {
        auto& editor = editors[i];
        ImGui::PushID(i);
        ImGui::Text("%s", editor->title().c_str());
        for (auto& field : editor->fields()) {
            if (render_editor_field(field)) editor->update();
        }
        ImGui::PopID();
    }
}

void GlfwImguiWindow::render_frame_() {
    loader_->make_current();
    // _gl glClear(_gl GL_COLOR_BUFFER_BIT);
    render();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(loader_->window_title().data());
    ImGui::SetWindowFontScale(2.f);

    ImGui::PushID(1);
    render_editor(editors_);
    ImGui::PopID();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    loader_->swap_buffers();
}

bool GlfwImguiWindow::event(AbsEvent* event) {
    auto e = static_cast<Event*>(event);
    if (e->sender() == this) {
        if (e->type() == EventType::Render) {
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
            close();
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
        } else if (e->type() == EventType::WindowResized) {
            return resize_event(static_cast<WindowResizeEvent*>(e));
        } else if (e->type() == EventType::Key) {
            return key_event(static_cast<KeyEvent*>(e));
        } else if (e->type() == EventType::Wheel) {
            return wheel_event(static_cast<WheelEvent*>(e));
        }
    }
    return false;
}
GlfwImguiWindow::~GlfwImguiWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
    loader_.reset();
}
void GlfwImguiWindow::render() {};
GlfwImguiWindow::GlfwImguiWindow(const std::string& title, int width,
                                 int height, int fps)
    : fps_(fps),
      AbsWindow(),
      loader_(std::make_unique<GlfwWindowLoader>(loop(), title.c_str(), width,
                                                 height)) {
    loader_->make_current();
    glbinding::initialize(loader_->get_proc_address(), false);
    ImGui::CreateContext();
    auto io = &ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(loader_->glfw_window_raw_ptr(), true);
    ImGui_ImplOpenGL3_Init();
}