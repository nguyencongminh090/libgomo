#include "pygomo/core/value.hpp"

#include <format>

namespace pygomo {
namespace {

std::string formatCoord(const Coord& c) { return std::format("{},{}", c.x, c.y); }

std::string formatMove(const Move& m) {
    return std::format("{},{},{}", m.coord.x, m.coord.y, m.side == Side::Black ? 1 : 2);
}

template <typename T, typename F> std::string joinSpaced(const std::vector<T>& items, F formatOne) {
    std::string out;
    for (const T& item : items) {
        if (!out.empty()) out += ' ';
        out += formatOne(item);
    }
    return out;
}

}  // namespace

std::string format(const Value& value) {
    struct Visitor {
        std::string operator()(bool v) const { return v ? "1" : "0"; }
        std::string operator()(std::int64_t v) const { return std::to_string(v); }
        std::string operator()(double v) const { return std::format("{}", v); }
        std::string operator()(const std::string& v) const { return v; }
        std::string operator()(const Coord& v) const { return formatCoord(v); }
        std::string operator()(const Move& v) const { return formatMove(v); }
        std::string operator()(const std::vector<Coord>& v) const {
            return joinSpaced(v, formatCoord);
        }
        std::string operator()(const std::vector<Move>& v) const {
            return joinSpaced(v, formatMove);
        }
    };
    return std::visit(Visitor{}, value);
}

}  // namespace pygomo
