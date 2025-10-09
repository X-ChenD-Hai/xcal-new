#ifdef USE_GLBINDING
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#define _gl gl::
#endif
#ifdef USE_GLAD
#include <glad/glad.h>
#define _gl
#endif
#include <print>

#include "./SceneWindow.hpp"

SceneWindow::SceneWindow(const std::string& name, int width, int height,
                         int fps)
    : GlfwImguiWindow(name, width, height, fps) {
    init_();
}
SceneWindow::SceneWindow() : GlfwImguiWindow() { init_(); }
void SceneWindow::render() { _gl glClear(_gl GL_COLOR_BUFFER_BIT); }
void SceneWindow::init_() {
    add_editor([this](std::string& name) { name_ = name; }, "global", "name",
               name_);
    add_editor(
        [this](float r, float g, float b) {
            color_ = {r, g, b};
            _gl glClearColor(r, g, b, 1.0f);
            std::print("clear color: {} {} {}\n", r, g, b);
        },
        "clear color", "r", 0.3f, "g", 0.3f, "b", 0.3f);
};