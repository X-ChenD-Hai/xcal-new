#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace node {
class Node;
using node_ptr = std::unique_ptr<Node>;
enum class PinValue {
    Float,
    Int,
    Bool,
};
struct Pin {
    PinValue value_type;
    std::string name;
};

struct PinAdapter {
    virtual size_t pin_count() const = 0;
    virtual const Pin* pin_at(size_t index) const = 0;
    ~PinAdapter() = default;
};

struct PropertyAdapter {
    virtual size_t property_count() const = 0;
    virtual std::function<void(node::Node*)> property_at(
        size_t index) const = 0;
    ~PropertyAdapter() = default;
};

class NodeClass;
class Node {
   public:
    virtual const PinAdapter* input_pin() { return nullptr; };
    virtual const PinAdapter* output_pin() { return nullptr; };
    virtual const PropertyAdapter* property_adapter() { return nullptr; };
    virtual const char* name() const { return nullptr; }

    Node(NodeClass* node_class) : node_class_(node_class) {}
    const char* class_name();
    virtual ~Node() = default;

   private:
    NodeClass* node_class_;
};

struct SingleValuePinAdapter : PinAdapter {
    SingleValuePinAdapter(PinValue value_type, std::string name)
        : pin_{value_type, std::move(name)} {}
    size_t pin_count() const override { return 1; }
    const Pin* pin_at(size_t index) const override { return &pin_; }

   private:
    Pin pin_;
};

class ConstValueNode : public Node {
   public:
    ConstValueNode(NodeClass* node_class, PinValue value_type, std::string name)
        : Node(node_class), pin_adapter_(value_type, std::move(name)) {}
    const PinAdapter* input_pin() override { return nullptr; }
    const PinAdapter* output_pin() override { return &pin_adapter_; }
    SingleValuePinAdapter pin_adapter_;
};

class NodeClass {
   public:
    virtual const char* name() const = 0;
    virtual node_ptr create_node() = 0;
    virtual ~NodeClass() = default;
};
using node_class_ptr = std::unique_ptr<NodeClass>;
class ConstValueNodeClass : public NodeClass {
    const char* name() const override { return ClassName; }
    node_ptr create_node() override {
        return std::make_unique<ConstValueNode>(this, PinValue::Float,
                                                "ConstValue");
    }
    static constexpr auto ClassName = "ConstValue";
};

class PrintNode : public Node {
   public:
    PrintNode(NodeClass* node_class) : Node(node_class) {}
    const PinAdapter* input_pin() override { return &pin_adapter_; }
    const PinAdapter* output_pin() override { return nullptr; }
    SingleValuePinAdapter pin_adapter_{PinValue::Float, "Print"};
};

class PrintNodeClass : public NodeClass {
    const char* name() const override { return ClassName; }
    node_ptr create_node() override {
        return std::make_unique<PrintNode>(this);
    }
    static constexpr auto ClassName = "Print";
};

}  // namespace node
