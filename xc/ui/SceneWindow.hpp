#pragma once
#include "./GlfwImguiWindow.hpp"

class SceneWindow : public GlfwImguiWindow {
   private:
    std::string name_ = "Scene Window";
    std::array<float,3> color_;

   public:
    SceneWindow();

   private:
    void render() override;
};
