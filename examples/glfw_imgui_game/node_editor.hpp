#pragma once
#include <imgui_node_editor.h>

#include <ecs/tyoes.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "./node_types.hpp"

struct NodeEditor {
    static void init(ecs::World& world);

    NodeEditor();

    ~NodeEditor();

    void update(ecs::World& world);

    void draw_nodes();

    void handle_create_link();
    void handle_delete_link();
    void handle_create_node();
    void handle_delete_node();
    void handle_context_menu();

    void popu_background_context();
    void popup_node_context();
    void popup_link_context();
    void popup_pin_context();


    // 节点定义
    struct Pin {
        std::string name{};
    };
    struct Node {
        std::vector<std::unique_ptr<Pin>> input_pins;
        std::vector<std::unique_ptr<Pin>> output_pins;
        template <class... Args>
        Pin* create_pin(ax::NodeEditor::PinKind kind, Args... args) {
            if (kind == ax::NodeEditor::PinKind::Input)
                return input_pins.emplace_back(new Pin{std::forward(args)...})
                    .get();

            return output_pins.emplace_back(new Pin{std::forward(args)...})
                .get();
        };
    };

    // 连接定义
    struct Link {
        ax::NodeEditor::PinId input_pin_id;
        ax::NodeEditor::PinId output_pin_id;
    };

   protected:
    Node* create_node() {
        nodes_.push_back(std::make_unique<Node>());
        return nodes_.back().get();
    }
    ax::NodeEditor::LinkId create_link(ax::NodeEditor::PinId input,
                                       ax::NodeEditor::PinId output) {
        links_.push_back(std::make_unique<Link>(input, output));
        return ax::NodeEditor::LinkId{links_.back().get()};
    }
    void delete_link(ax::NodeEditor::LinkId id) {
        links_.erase(std::remove_if(links_.begin(), links_.end(),
                                    [id](const auto& link) {
                                        return link.get() == id.AsPointer();
                                    }),
                     links_.end());
    }

   public:
    bool show{true};

   private:
    ax::NodeEditor::EditorContext* editor_context_{nullptr};
    std::vector<std::unique_ptr<Node>> nodes_{};
    std::vector<std::unique_ptr<Link>> links_{};
    ImVec2 open_popu_pos_{};
    ax::NodeEditor::NodeId context_node_{};
    ax::NodeEditor::LinkId context_link_{};
    ax::NodeEditor::PinId context_pin_;

};
