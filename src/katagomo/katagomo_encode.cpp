#include "gomo/katagomo/katagomo_encode.hpp"

#include <string>
#include <vector>

namespace gomo::encode {
namespace {

const char* ruleCommand(BoardRule r) {
    switch (r) {
    case BoardRule::Freestyle: return "FREESTYLE";
    case BoardRule::Standard: return "STANDARD";
    case BoardRule::Renju: return "RENJU";
    }
    return "FREESTYLE";
}

}  // namespace

EncodedRequest defaultKataGomoRequest(const SearchRequest& request) {
    const Rules&    rules    = request.rules;
    const Position& position = request.position;
    const Limits&   limits   = request.limits;
    if (rules.opening != OpeningRule::FreeOpen || !rules.extensions.empty() ||
        position.layout() != BoardLayout{} || position.width() != position.height() ||
        limits.matchTime || limits.maxDepth) {
        return std::unexpected(EngineError::Unsupported);
    }

    // START resets the board, so the settings follow it.
    std::vector<std::string> lines;
    lines.push_back("START " + std::to_string(position.width()));
    lines.push_back(ruleCommand(rules.board));
    if (rules.maxMoves) lines.push_back("maxmoves " + std::to_string(*rules.maxMoves));
    if (limits.turnTime) {
        lines.push_back("INFO timeout_turn " + std::to_string(limits.turnTime->count()));
    }
    if (limits.maxNodes) {
        lines.push_back("kata-set-param maxVisits " + std::to_string(*limits.maxNodes));
    }
    // BOARD alternates colours from Black and ignores any third field; an empty list is the
    // empty board.
    lines.push_back("BOARD");
    for (const Move& m : position.moves()) {
        lines.push_back(std::to_string(m.coord.x) + "," + std::to_string(m.coord.y));
    }
    lines.push_back("DONE");
    return lines;
}

}  // namespace gomo::encode
