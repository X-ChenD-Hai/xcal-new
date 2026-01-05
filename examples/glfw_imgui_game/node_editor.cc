#include "./node_editor.hpp"

#include <algorithm>
#include <cstddef>
#include <ecs/world.hpp>
#include <memory>
#include <print>

#include "imgui.h"
#include "imgui_node_editor.h"

namespace ed = ::ax::NodeEditor;
using PK = ed::PinKind;

namespace details {
static constexpr auto kEdNodeContextPopuId = "kedNodeContextPopuId";
static constexpr auto kEdLinkContextPopuId = "kedLinkContextPopuId";
static constexpr auto kEdPinContextPopuId = "kedPinContextPopuId";
static constexpr auto kEdBackgroundContextPopuId = "kedBackgroundContextPopuId";

}  // namespace details

void NodeEditor::init(ecs::World& world) { world.add_resource<NodeEditor>(); }
NodeEditor::NodeEditor() {
    // 创建节点编辑器上下文
    ed::Config config;

    config.SettingsFile = nullptr;

    editor_context_ = ed::CreateEditor(&config);
    auto n1 = create_node();
    n1->create_pin(PK::Input);
    n1->create_pin(PK::Output);

    auto n2 = create_node();
    n2->create_pin(PK::Input);
    n2->create_pin(PK::Output);
}

NodeEditor::~NodeEditor() {
    // 销毁节点编辑器上下文
    ed::DestroyEditor(editor_context_);
}

void NodeEditor::update(ecs::World& world) {
    using namespace ImGui;

    // 设置当前编辑器上下文
    ed::SetCurrentEditor(editor_context_);

    // 开始节点编辑器
    ed::Begin("Node Editor");

    // 创建一些示例节点
    draw_nodes();

    // 结束节点编辑器
    ed::End();

    // 重置当前编辑器上下文
    ed::SetCurrentEditor(nullptr);
}

void NodeEditor::draw_nodes() {
    using namespace ImGui;

    // 创建节点
    for (auto& node : nodes_) {
        auto id = ed::NodeId(node.get());
        ed::BeginNode(id);

        // 节点标题
        ImGui::Text("Node %p", id.AsPointer());

        auto input_size = node->input_pins.size();
        auto output_size = node->output_pins.size();
        size_t max_pins = std::max(input_size, output_size);
        for (size_t i = 0; i < max_pins; i++) {
            if (i < input_size) {
                ed::PinId input_pin_id{node->input_pins[i].get()};
                ed::BeginPin(input_pin_id, ed::PinKind::Input);
                ImGui::Text("Input");
                ed::EndPin();
            }
            ImGui::SameLine();
            if (i < output_size) {
                ed::PinId output_pin_id{node->output_pins[i].get()};
                ed::BeginPin(output_pin_id, ed::PinKind::Output);
                ImGui::Text("Output");
                ed::EndPin();
            }
        }
        ed::EndNode();
    }

    // 创建连接
    for (auto& link : links_) {
        ed::Link(ax::NodeEditor::LinkId(link.get()), link->input_pin_id,
                 link->output_pin_id);
    }

    // 处理新的连接
    handle_create_link();

    // 处理删除连接
    handle_delete_link();

    handle_context_menu();
}

void NodeEditor::handle_create_link() {
    // 开始连接
    if (ed::BeginCreate()) {
        ed::PinId from_pin_id, to_pin_id;
        if (ed::QueryNewLink(&from_pin_id, &to_pin_id)) {
            // 验证连接
            if (from_pin_id && to_pin_id) {
                // 接受连接
                ed::AcceptNewItem(ImColor(255, 255, 255));
                // 添加新连接
                create_link(from_pin_id, to_pin_id);
            }
        }
    }
    ed::EndCreate();
}

void NodeEditor::handle_delete_link() {
    // 处理删除连接
    if (ed::BeginDelete()) {
        std::println("begin delete");
        ed::LinkId deleted_link_id;
        while (ed::QueryDeletedLink(&deleted_link_id))
            // 从连接列表中删除
            if (ed::AcceptDeletedItem()) delete_link(deleted_link_id);
    }
    ed::EndDelete();
}

void NodeEditor::handle_create_node() {}
void NodeEditor::handle_delete_node() {}

void NodeEditor::handle_context_menu() {
    open_popu_pos_ = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup(details::kEdBackgroundContextPopuId);
    } else if (ed::ShowNodeContextMenu(&context_node_)) {
        ImGui::OpenPopup(details::kEdNodeContextPopuId);
    } else if (ed::ShowLinkContextMenu(&context_link_)) {
        ImGui::OpenPopup(details::kEdLinkContextPopuId);
    } else if (ed::ShowPinContextMenu(&context_pin_)) {
        ImGui::OpenPopup(details::kEdPinContextPopuId);
    }
    ed::Resume();
    ed::Suspend();
    if (ImGui::BeginPopup(details::kEdBackgroundContextPopuId)) {
        popu_background_context();
        ImGui::EndPopup();
    } else if (ImGui::BeginPopup(details::kEdNodeContextPopuId)) {
        popup_node_context();
        ImGui::EndPopup();
    } else if (ImGui::BeginPopup(details::kEdLinkContextPopuId)) {
        popup_link_context();
        ImGui::EndPopup();
    } else if (ImGui::BeginPopup(details::kEdPinContextPopuId)) {
        popup_pin_context();
        ImGui::EndPopup();
    }

    ed::Resume();
}

void NodeEditor::popu_background_context() {
    if (ImGui::BeginMenu("Create Node")) {
        // create_node();
        std::println("Create Node");
        if (ImGui::MenuItem("react")) {
            auto node = create_node();
            ed::SetNodePosition(ed::NodeId{node}, open_popu_pos_);
            std::println("react");
        }
        ImGui::EndMenu();
    }
}

void NodeEditor::popup_node_context() {}

void NodeEditor::popup_link_context() {}

void NodeEditor::popup_pin_context() {}
