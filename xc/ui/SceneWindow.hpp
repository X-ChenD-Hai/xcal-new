#pragma once
#include <ecs/World.hpp>
#include <glm/glm.hpp>

#include "./GlfwImguiWindow.hpp"
#include "ecs/EventBus.hpp"

struct WorldRequestExitEvent {};
struct WorldeadyToExitEvent {};
struct Shader;
class UiEditorCacher;
class SceneWindow : public GlfwImguiWindow {
   private:
    std::string name_ = "Scene Window";
    std::array<float, 3> color_;
    ecs::World world_;
    bool world_ready_stop_ = false;
    ecs::EventBus event_bus_;
    std::unique_ptr<UiEditorCacher> ui_editor_cacher_;
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders_;

   public:
    SceneWindow();
    SceneWindow(const std::string& name, int width, int height, int fps);
    ~SceneWindow() override;
    bool event(AbsEvent* event) override;

   private:
    void render() override;

   private:
    void init_();
    void init_editors_();
    void init_world_();
    void add_component_to_world_();
    void create_trangle_entity_();
    void update_world_();
};
