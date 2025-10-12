#pragma once
#include <ecs/World.hpp>
#include <glm/glm.hpp>

#include "./GlfwImguiWindow.hpp"
#include "ecs/EventBus.hpp"


struct WorldRequestExitEvent {};
struct WorldeadyToExitEvent {};

class UiEditorCacher;
class SceneWindow : public GlfwImguiWindow {
   private:
    std::string name_ = "Scene Window";
    std::array<float, 3> color_;
    ecs::World world_;
    bool world_ready_stop_ = false;
    ecs::EventBus event_bus_;
    std::unique_ptr<UiEditorCacher> ui_editor_cacher_;
   public:
    SceneWindow();
    SceneWindow(const std::string& name, int width, int height, int fps);
    ~SceneWindow() override;
    bool event(AbsEvent* event) override;

   private:
    void render() override;
    void init_();
    void add_component_to_world_();
};
