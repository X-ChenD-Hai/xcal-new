#include "./node_editor.hpp"

#include <algorithm>
#include <cstddef>
#include <ecs/world.hpp>
#include <memory>
#include <print>

#include "imgui.h"
#include "imgui_node_editor.h"
#include "node_types.hpp"

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
    node_class_.emplace_back(std::make_unique<node::ConstValueNodeClass>());
    nodes_.emplace_back(node_class_.back()->create_node());
    node_class_.emplace_back(std::make_unique<node::PrintNodeClass>());
    nodes_.emplace_back(node_class_.back()->create_node());
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
    for (auto& node : nodes_) {
        auto id = ed::NodeId(node.get());
        ed::BeginNode(id);
        draw_node(node.get());
        ed::EndNode();
    }
    for (auto& link : links_) {
        ed::Link(ax::NodeEditor::LinkId(link.get()), link->input_pin_id,
                 link->output_pin_id);
    }
    handle_create_link();
    handle_delete_link();
    handle_context_menu();
    // 结束节点编辑器
    ed::End();

    // 重置当前编辑器上下文
    ed::SetCurrentEditor(nullptr);
}

void NodeEditor::draw_node(node::Node* node) {
    using namespace ImGui;

    // Draw node header with background
    const auto header_text =
        node->name() ? node->name() : std::string{node->class_name()} + " Node";
    auto text_size = ImGui::CalcTextSize(header_text.c_str());

    // Add some padding to the background
    ImVec2 padding = ImVec2(8.0f, 4.0f);
    ImVec2 bg_size =
        ImVec2(text_size.x + padding.x * 2, text_size.y + padding.y * 2);

    // Get the current cursor position before drawing anything
    auto cursor_pos = ImGui::GetCursorScreenPos();

    // Draw background using window draw list with screen coordinates
    auto draw_list = ImGui::GetWindowDrawList();

    // Draw filled rectangle with darker color
    draw_list->AddRectFilled(
        cursor_pos, ImVec2(cursor_pos.x + bg_size.x, cursor_pos.y + bg_size.y),
        IM_COL32(0, 10, 150, 255),  // Darker gray color
        4.0f                        // Add some rounding
    );

    // Draw border for more visibility
    draw_list->AddRect(
        cursor_pos, ImVec2(cursor_pos.x + bg_size.x, cursor_pos.y + bg_size.y),
        IM_COL32(120, 120, 120, 255),  // Lighter gray for border
        4.0f,
        0,    // No specific corners
        1.5f  // Border thickness
    );

    // Add padding before text
    ImGui::SetCursorScreenPos(
        ImVec2(cursor_pos.x + padding.x, cursor_pos.y + padding.y));

    // Draw the text with white color for contrast
    ImGui::TextUnformatted(header_text.c_str());

    // Reset cursor position after drawing text
    ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x, cursor_pos.y + bg_size.y));

    // Draw the rest of the node content
    if (node->input_pin()) {
        BeginGroup();
        draw_input_adapter(node->input_pin(), ed::NodeId{node});
        EndGroup();
        SameLine();
    }
    if (node->property_adapter()) {
        BeginGroup();
        draw_node_properties(node);
        EndGroup();
        SameLine();
    }
    if (node->output_pin()) {
        BeginGroup();
        draw_output_adapter(node->output_pin(), ed::NodeId{node});
        EndGroup();
    }
}
void NodeEditor::draw_node_header(node::Node* node) {
    // 计算文字高度并垂直居中
    using namespace ImGui;
    const auto text =
        node->name() ? node->name() : std::string{node->class_name()} + " Node";
    auto text_size = ImGui::CalcTextSize(text.c_str());

    ImGui::TextUnformatted(text.c_str());
    ImGui::Spacing();
}

void NodeEditor::draw_input_adapter(const node::PinAdapter* pin_adapter,
                                    ed::NodeId node_id) {
    if (!pin_adapter) return;
    for (size_t i = 0; i < pin_adapter->pin_count(); i++) {
        ed::PinId input_pin_id{pin_adapter->pin_at(i)};
        ed::BeginPin(input_pin_id, ed::PinKind::Input);
        ImGui::TextUnformatted(pin_adapter->pin_at(i)->name.c_str());
        ed::EndPin();
        ImGui::SameLine();
    }
}
void NodeEditor::draw_output_adapter(const node::PinAdapter* pin_adapter,
                                     ed::NodeId node_id) {
    if (!pin_adapter) return;
    for (size_t i = 0; i < pin_adapter->pin_count(); i++) {
        ed::PinId output_pin_id{pin_adapter->pin_at(i)};
        auto text = pin_adapter->pin_at(i)->name.c_str();
        ed::BeginPin(output_pin_id, ed::PinKind::Output);
        ImGui::TextUnformatted(text);
        ed::EndPin();
        ImGui::SameLine();
    }
}
void NodeEditor::draw_node_properties(node::Node* node) {
    if (!node) return;
    auto property_adapter = node->property_adapter();
    if (!property_adapter) return;
    for (size_t i = 0; i < property_adapter->property_count(); i++) {
        property_adapter->property_at(i)(node);
    }
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
        size_t unique_id = 0;
        for (auto& node_class : node_class_) {
            ImGui::PushID(++unique_id);
            if (ImGui::MenuItem(node_class->name())) {
                auto node = node_class->create_node();
                ed::SetNodePosition(ed::NodeId{node.get()}, open_popu_pos_);
                std::println("{}", node_class->name());
                nodes_.push_back(std::move(node));
            }
            ImGui::PopID();
        }
        ImGui::EndMenu();
    }
}

void NodeEditor::popup_node_context() {}

void NodeEditor::popup_link_context() {}

void NodeEditor::popup_pin_context() {}
ax::NodeEditor::LinkId NodeEditor::create_link(ax::NodeEditor::PinId input,
                                               ax::NodeEditor::PinId output) {
    links_.push_back(std::make_unique<Link>(input, output));
    return ax::NodeEditor::LinkId{links_.back().get()};
}
void NodeEditor::delete_link(ax::NodeEditor::LinkId id) {
    links_.erase(std::remove_if(links_.begin(), links_.end(),
                                [id](const auto& link) {
                                    return link.get() == id.AsPointer();
                                }),
                 links_.end());
}
