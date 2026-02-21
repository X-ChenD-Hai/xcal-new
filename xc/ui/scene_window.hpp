#pragma once
#include <xc/ecs/world.hpp>

#include "./glfw_imgui_window.hpp"
#include <xc/ecs/event_bus.hpp>
namespace ecs {
class ResourceTable;
}
struct WorldRequestExitEvent {};
struct WorldeadyToExitEvent {};
class UiEditorCacher;
class SceneWindow : public GlfwImguiWindow {
   private:
    std::string name_{"Scene Window"};
    std::array<float, 3> color_;
    ecs::World world_;
    bool world_ready_stop_ = false;
    ecs::EventBus event_bus_;
    std::unique_ptr<UiEditorCacher> ui_editor_cacher_;
    struct {
        double x_pos = 0.0;
        double y_pos = 0.0;
    } last_mouse_pos_;
    bool moving = false;

   public:
    SceneWindow();
    SceneWindow(const std::string& name, int width, int height, int fps);

   public:
    ~SceneWindow() override;
    bool event(AbsEvent* event) override;
    bool resize_event(WindowResizeEvent*) override;
    bool key_event(KeyEvent*) override;
    bool wheel_event(WheelEvent*) override;
    bool mouse_move_event(MouseMoveEvent*) override;
    bool mouse_button_event(MouseButtonEvent*) override;

   private:
    void render() override;

   private:
    void init_();
    void init_editors_();
    void init_world_();
    void create_entity_();
    void update_world_();
};
