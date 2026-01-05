#pragma once
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
};

class NodeClass;
class Node {
    public:
    virtual const PinAdapter* input_pin(size_t index) = 0;
    virtual const PinAdapter* output_pin(size_t index) = 0;
    virtual const char* name() const { return nullptr; }

    Node(NodeClass* node_class) : node_class_(node_class) {}

   private:
    NodeClass* node_class_;
};

struct ConstValuePinAdapter : PinAdapter {
    ConstValuePinAdapter(PinValue value_type, std::string name)
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
    const PinAdapter* input_pin(size_t index) override { return nullptr; }
    const PinAdapter* output_pin(size_t index) override {
        return &pin_adapter_;
    }
    ConstValuePinAdapter pin_adapter_;
};

class NodeClass {
    virtual std::string_view name() const = 0;
    virtual node_ptr create_node() = 0;
};

class ConstValueNodeClass : public NodeClass {
    std::string_view name() const override { return "ConstValue"; }
    node_ptr create_node() override {
        return std::make_unique<ConstValueNode>(this, PinValue::Float, "ConstValue");
    }
};

}  // namespace node
