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
    // auto tmp = field.value;

    auto render = [&]<typename T, typename Fn>(EditorField& field, T& val,
                                               Fn&& r) {
        try {
            val = std::get<T>(field.value);
        } catch (...) {
        }
        if (r(field.name.c_str(), &val)) {
            field.value = val;
            return true;
        }
        return false;
    };

    switch (field.type) {
        case EditFieldType::String: {
            static char buf[1024];
            fill(buf, buf + 1024, '\0');
            try {
                string str = std::get<std::string>(field.value);
                strcpy_s(buf, str.c_str());
            } catch (...) {
            }
            if (ImGui::InputText(field.name.c_str(), buf, 1024)) {
                field.value = std::string(buf);
                std::println("set val: {} from {}",std::get<std::string>(field.value),(void*)&field);
                return true;
            }
            break;
        }
        case EditFieldType::Int: {
            auto val = 0;
            return render(field, val, [&](auto name, auto* val) {
                // std::println("render int: {}",std::get<int>(field.value) );
                return ImGui::InputInt(name, val);
            });
            break;
        }
        case EditFieldType::Float: {
            auto val = 0.f;
            return render(field, val, [](auto name, auto* val) {
                return ImGui::InputFloat(name, val);
            });
            break;
        }

        case EditFieldType::Bool: {
            auto val = false;
            return render(field, val, [](auto name, auto* val) {
                return ImGui::Checkbox(name, val);
            });
            break;
        }
    }

    return false;
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

void Window::render_frame_() {
    loader_->make_current();
    _gl glClear(_gl GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Hello, world!");
    ImGui::SetWindowFontScale(2.f);

    ImGui::PushID(1);
    render_editor(editors_);
    ImGui::PopID();
    ImGui::End();

    render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    loader_->swap_buffers();
}

bool Window::event(AbsEvent* event) {
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
Window::Window()
    : AbsWindow(), loader_(std::make_unique<GlfwWindowLoader>(loop())) {
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
void Window::render() {};