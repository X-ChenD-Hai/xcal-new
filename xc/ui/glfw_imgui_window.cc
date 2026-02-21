#include "./glfw_imgui_window.hpp"
#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#define _gl gl::
#endif
#ifdef USE_GLAD
#include <glad/gl.h>
#define _gl
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ranges>

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

static void render_editor(GlfwImguiWindow::editor_list& editors) {
    for (int i = 0; i < editors.size(); i++) {
        auto& editor = editors[i].first;
        ImGui::PushID(i);
        ImGui::Text("%s", editor->title().c_str());
        auto width = ImGui::GetWindowWidth();
        auto item_width = width / editor->fields().size();

        ImGui::PushItemWidth(item_width);
        for (size_t idx = 0; idx < editor->fields().size(); idx++) {
            auto& field = editor->fields()[idx];
            if (render_editor_field(field)) editor->update();
            if (idx != editor->fields().size() - 1) {
                if (editors[i].second) {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
    }
}

static void render_buttons(std::vector<std::unique_ptr<Button>>& buttons) {
    for (int i = 0; i < buttons.size(); i++) {
        auto& button = buttons[i];
        ImGui::PushID(i);
        if (ImGui::Button(button->title().c_str())) {
            button->click();
        }
        ImGui::PopID();
    }
}

void GlfwImguiWindow::render_frame_() {
    loader_->make_current();
    render();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(loader_->window_title().data());
    ImGui::SetWindowFontScale(2.f);

    for (size_t i = 0; i < ui_elements_.size(); i++) {
        ImGui::PushID(i);
        std::visit(
            [](auto&& element) {
                using T = std::decay_t<decltype(element)>;
                if constexpr (std::is_same_v<T, editor_list>) {
                    render_editor(element);
                } else if constexpr (std::is_same_v<T, button_list>) {
                    render_buttons(element);
                }
            },
            ui_elements_[i]);
        ImGui::PopID();
    }

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
#ifdef USE_GLBINDING
    glbinding::initialize(loader_->get_proc_address(), false);
#elif defined(USE_GLAD)
    gladLoadGL(loader_->get_proc_address());
#endif
    ImGui::CreateContext();
    auto io = &ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(loader_->glfw_window_raw_ptr(), true);
    ImGui_ImplOpenGL3_Init();
}
template <typename T>
T& GlfwImguiWindow::append_element(T&& element) {
    if (ui_elements_.empty()) {
        ui_elements_.emplace_back(std::vector<T>{});
        std::get<std::vector<T>>(ui_elements_.back())
            .emplace_back(std::move(element));
        return std::get<std::vector<T>>(ui_elements_.back()).back();
    }
    if (auto it = std::get_if<std::vector<T>>(&ui_elements_.back())) {
        it->emplace_back(std::move(element));
    } else {
        ui_elements_.emplace_back(std::vector<T>{});
        std::get<std::vector<T>>(ui_elements_.back())
            .emplace_back(std::move(element));
    }
    return std::get<std::vector<T>>(ui_elements_.back()).back();
}
template std::unique_ptr<Button>& GlfwImguiWindow::append_element(
    std::unique_ptr<Button>&& element);
template std::pair<std::unique_ptr<Editor>, bool>&
GlfwImguiWindow::append_element(
    std::pair<std::unique_ptr<Editor>, bool>&& element);
void GlfwImguiWindow::set_cursor_mode(CursorMode mode) {
    loader_->set_cursor_mode(mode);
}
