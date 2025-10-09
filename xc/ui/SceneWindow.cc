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

SceneWindow::SceneWindow() : GlfwImguiWindow() {
    _gl glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    add_editor([this](std::string& name) { name_ = name; }, "global", "name",
               name_);
    add_editor(
        [this](float r, float g, float b) {
            color_ = {r, g, b};
            _gl glClearColor(r, g, b, 1.0f);
            std::print("clear color: {} {} {}\n", r, g, b);
        },
        "clear color", "r", color_[0], "g", color_[1], "b", color_[2]);
}
void SceneWindow::render() { _gl glClear(_gl GL_COLOR_BUFFER_BIT); }
