#include "gomo/gomocup/gomocup_encode.hpp"

namespace gomo::encode {

std::optional<int> gomocupRuleId(const Rules& r) {
    if (r.opening == OpeningRule::FreeOpen) {
        switch (r.board) {
        case BoardRule::Freestyle: return 0;
        case BoardRule::Standard: return 1;
        case BoardRule::Renju: return 4;
        }
    }
    if (r.board == BoardRule::Freestyle) { return r.opening == OpeningRule::Swap1 ? 5 : 6; }
    return std::nullopt;
}

std::expected<std::vector<std::string>, EngineError> defaultGomocupRequest(
    const SearchRequest& request) {
    const Rules&    rules    = request.rules;
    const Position& position = request.position;
    const Limits&   limits   = request.limits;
    const auto      rule     = gomocupRuleId(rules);
    if (!rule || !rules.extensions.empty() || position.layout() != BoardLayout{}) {
        return std::unexpected(EngineError::Unsupported);
    }

    std::vector<std::string> lines;
    if (position.width() == position.height()) {
        lines.push_back("START " + std::to_string(position.width()));
    } else {
        lines.push_back("RECTSTART " + std::to_string(position.width()) + "," +
                        std::to_string(position.height()));
    }
    lines.push_back("INFO rule " + std::to_string(*rule));
    if (rules.maxMoves) lines.push_back("INFO max_moves " + std::to_string(*rules.maxMoves));
    if (limits.turnTime) {
        lines.push_back("INFO timeout_turn " + std::to_string(limits.turnTime->count()));
    }
    if (limits.matchTime) {
        lines.push_back("INFO timeout_match " + std::to_string(limits.matchTime->count()));
    }
    if (limits.maxDepth) lines.push_back("INFO max_depth " + std::to_string(*limits.maxDepth));
    if (limits.maxNodes) lines.push_back("INFO max_node " + std::to_string(*limits.maxNodes));
    if (position.moves().empty()) {
        lines.push_back("BEGIN");
    } else {
        auto block = boardBlock("BOARD", position.moves());
        lines.insert(lines.end(), block.begin(), block.end());
    }
    return lines;
}

std::vector<std::string> boardBlock(std::string_view command, std::span<const Move> moves,
                                    std::span<const std::string> extra) {
    std::vector<std::string> lines;
    lines.reserve(moves.size() + extra.size() + 2);
    lines.emplace_back(command);
    // Same rule as Position::sideToMove: Black moves after an even number of moves.
    const Side mine = moves.size() % 2 == 0 ? Side::Black : Side::White;
    for (const Move& m : moves) {
        lines.push_back(std::to_string(m.coord.x) + "," + std::to_string(m.coord.y) + "," +
                        (m.side == mine ? "1" : "2"));
    }
    lines.insert(lines.end(), extra.begin(), extra.end());
    lines.emplace_back("DONE");
    return lines;
}

}  // namespace gomo::encode
