#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xc::cmdline {

inline std::string offset_print_string(std::string_view str, size_t offset,
                                       size_t width = std::string::npos,
                                       std::string_view space = " ") {
    std::string offset_str;
    std::string result(str);
    for (int i = 0; i < offset; i++) {
        offset_str += space;
    }
    if (width == std::string::npos) return offset_str + result;
    auto limit = width - offset_str.length();
    assert(limit > 0);
    int pos = 0;
    while (pos < result.length()) {
        if (pos < result.length() && pos != 0) {
            auto c = (uint8_t)result[pos];
            if ((c & (uint8_t)(0x01 << 7)) != 0) {
                while ((c & (uint8_t)(0x11 << 6)) != (0x11 << 6)) {
                    pos++;
                    if (pos >= result.length()) break;
                    c = (uint8_t)result[pos];
                }
            }
        }
        result.insert(pos, offset_str);
        if (pos != 0) result.insert(pos, "\n");
        pos += offset_str.length() + limit;
    }
    return result;
}

struct Enum {
    struct EnumValue {
        std::string name;
        std::optional<std::string> short_name;
        std::string description;
    };
    std::vector<EnumValue> enum_values;
};

struct BoolOption {
    std::string name;
    std::vector<std::string> options;
    std::string description;
};
struct Option : public BoolOption {
    std::vector<std::string> values;
};

struct EnumOption : public Option {};

struct OptionToken {
    std::string name;
};
struct ValueToken {
    std::string value;
};
struct TransitionToken {
    bool is_short;
};
class Command;
class CommandSet {
    friend class Command;
    std::string name_;
    std::string description_;
    std::vector<std::unique_ptr<Command>> commands_;
};

class Command {
    int argc_;
    int offset_;
    char** argv_;
    Command* parent_;
    std::string name_;
    std::vector<std::string> short_names_;
    std::string description_;
    std::vector<std::variant<OptionToken, ValueToken, TransitionToken>> tokens_;
    std::vector<std::unique_ptr<Command>> subcommands_;
    std::vector<CommandSet> command_sets_;
    std::unordered_map<std::string_view, Command*> command_map_;

   public:
    Command(int argc, char** argv) : argc_(argc), argv_(argv) {
        std::cout << "Parser created" << std::endl;
        for (int i = 0; i < argc; i++) {
            auto arg = std::string_view{argv[i]};
            std::cout << "[" << i << "]: " << arg
                      << "  : len= " << arg.length();
            if (arg.starts_with("--")) {
                if (arg.length() > 2)
                    std::cout << " is option";
                else
                    std::cout << " inner transition option";
            } else if (arg.starts_with("-")) {
                if (arg.length() > 1)
                    std::cout << " is short option";
                else
                    std::cout << " inner transition option";
            } else {
                try {
                    auto value = std::stoll(arg.data());
                    std::cout << " is num: " << value;
                } catch (std::invalid_argument& e) {
                    try {
                        auto value = std::stod(arg.data());
                        std::cout << " is float: " << value;
                    } catch (std::invalid_argument& e) {
                        std::cout << " is string";
                        if (command_map_.find(arg) != command_map_.end()) {
                            std::cout << " is subcommand";
                        }
                    }
                }
            }
            std::cout << std::endl;
        }
    };
    Command* add_command(Command&& command) {
        auto sub_command =
            subcommands_
                .emplace_back(std::make_unique<Command>(std::move(command)))
                .get();
        if (sub_command->parent_) assert("Command already has a parent");
        sub_command->parent_ = this;
        if (sub_command->name_.empty()) assert("Command name is empty");
        if (command_map_.find(sub_command->name_) != command_map_.end())
            assert("Command already exists");
        command_map_.insert({sub_command->name_, sub_command});
        for (auto& name : sub_command->short_names_) {
            if (name.empty()) assert("Command short name is empty");
            if (command_map_.find(name) != command_map_.end())
                assert("Command already exists");
            command_map_.insert({name, sub_command});
        }
        return sub_command;
    }
    void add_command_set(CommandSet&& command_set) {
        command_sets_.emplace_back(std::move(command_set));
        for (auto& command : command_set.commands_) {
            add_command(std::move(*command));
        }
    }
    void help() {}
};

};  // namespace xc::cmdline