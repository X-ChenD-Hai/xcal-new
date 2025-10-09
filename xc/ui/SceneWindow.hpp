#pragma once
#include "./GlfwImguiWindow.hpp"

class SceneWindow : public GlfwImguiWindow {
   private:
    std::string name_ = "Scene Window";
    std::array<float, 3> color_;

   public:
    SceneWindow();
    SceneWindow(const std::string& name, int width, int height, int fps);

   private:
    void render() override;
    void init_();
};
