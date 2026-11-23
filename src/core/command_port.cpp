#include "pygomo/core/command_port.hpp"

namespace pygomo {

std::vector<std::string> commandLines(const Command& command) {
    std::vector<std::string> out;
    if (command.lines.empty()) {
        std::string head = command.name;
        for (const Value& arg : command.args) {
            if (!head.empty()) head += ' ';
            head += format(arg);
        }
        if (!head.empty()) out.push_back(std::move(head));
    } else {
        out = command.lines;
    }
    for (const Value& entry : command.block) out.push_back(format(entry));
    if (command.terminator) out.push_back(*command.terminator);
    return out;
}

}  // namespace pygomo
