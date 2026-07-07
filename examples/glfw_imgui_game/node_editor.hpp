#pragma once
#include <imgui_node_editor.h>

#include <memory>
#include <vector>
#include <xc/ecs/tyoes.hpp>

#include "./node_types.hpp"

struct NodeEditor {
    static void init(ecs::World& world);

    NodeEditor();

    ~NodeEditor();

    void update(ecs::World& world);

    // 连接定义
    struct Link {
        ax::NodeEditor::PinId input_pin_id;
        ax::NodeEditor::PinId output_pin_id;
    };

   protected:
    void draw_node(node::Node* node);
    void draw_node_header(node::Node* node);
    void draw_input_adapter(const node::PinAdapter* pin_adapter,
                            ax::NodeEditor::NodeId node_id);
    void draw_node_properties(node::Node* node);
    void draw_output_adapter(const node::PinAdapter* pin_adapter,
                             ax::NodeEditor::NodeId node_id);

    void handle_create_link();
    void handle_delete_link();
    void handle_create_node();
    void handle_delete_node();
    void handle_context_menu();

    void popu_background_context();
    void popup_node_context();
    void popup_link_context();
    void popup_pin_context();

    ax::NodeEditor::LinkId create_link(ax::NodeEditor::PinId input,
                                       ax::NodeEditor::PinId output);
    void delete_link(ax::NodeEditor::LinkId id);

   public:
    bool show{true};

   private:
    std::vector<node::node_class_ptr> node_class_{};
    std::vector<node::node_ptr> nodes_{};
    std::vector<std::unique_ptr<Link>> links_{};
    // state
    ax::NodeEditor::EditorContext* editor_context_{nullptr};
    ImVec2 open_popu_pos_{};
    ax::NodeEditor::NodeId context_node_{};
    ax::NodeEditor::LinkId context_link_{};
    ax::NodeEditor::PinId context_pin_{};
};
